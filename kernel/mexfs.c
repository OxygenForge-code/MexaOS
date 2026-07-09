/* ============================================================
 *  MexaOS File System (.mex)
 *  Intent-aware native filesystem with AI metadata
 * ============================================================ */

#include "include/mexfs.h"
#include "include/memory.h"
#include "include/vga.h"
#include "include/ai_engine.h"

static uint8_t *fs_base = NULL;
static uint8_t *inode_bitmap = NULL;
static uint8_t *data_bitmap = NULL;
static struct mex_inode *inode_table = NULL;
static uint8_t *data_blocks = NULL;
static struct mex_superblock *super = NULL;

#define INODE_TABLE_SIZE    (MEX_MAX_FILES * sizeof(struct mex_inode))
#define INODE_BITMAP_SIZE   DIV_ROUND_UP(MEX_MAX_FILES, 8)
#define DATA_BITMAP_SIZE    DIV_ROUND_UP(65536, 8)
#define DATA_BLOCKS_SIZE    (65536 * MEX_BLOCK_SIZE)

#define MAX_OPEN_FILES  256
static struct mex_file open_files[MAX_OPEN_FILES];
static int open_file_count = 0;
static fid_t next_fid = 1;

static int inode_bitmap_test(uint64_t inum) {
    return (inode_bitmap[inum / 8] >> (inum % 8)) & 1;
}
static void inode_bitmap_set(uint64_t inum) {
    inode_bitmap[inum / 8] |= (1 << (inum % 8));
    super->free_inodes--;
}
static void inode_bitmap_clear(uint64_t inum) {
    inode_bitmap[inum / 8] &= ~(1 << (inum % 8));
    super->free_inodes++;
}
static int data_bitmap_test(uint64_t bnum) {
    return (data_bitmap[bnum / 8] >> (bnum % 8)) & 1;
}
static void data_bitmap_set(uint64_t bnum) {
    data_bitmap[bnum / 8] |= (1 << (bnum % 8));
    super->free_blocks--;
}
static void data_bitmap_clear(uint64_t bnum) {
    data_bitmap[bnum / 8] &= ~(1 << (bnum % 8));
    super->free_blocks++;
}
static uint64_t alloc_inode(void) {
    for (uint64_t i = 1; i < super->total_inodes; i++)
        if (!inode_bitmap_test(i)) { inode_bitmap_set(i); return i; }
    return 0;
}
static uint64_t alloc_block(void) {
    for (uint64_t i = 0; i < super->total_blocks; i++)
        if (!data_bitmap_test(i)) { data_bitmap_set(i); return i; }
    return 0;
}
static void free_block(uint64_t bnum) {
    if (bnum < super->total_blocks) data_bitmap_clear(bnum);
}
static struct mex_inode *get_inode(uint64_t inum) {
    if (inum >= super->total_inodes) return NULL;
    return &inode_table[inum];
}
static void write_inode(uint64_t inum, struct mex_inode *inode) {
    if (inum < super->total_inodes)
        memcpy(&inode_table[inum], inode, sizeof(struct mex_inode));
}
static uint8_t *get_block(uint64_t bnum) {
    if (bnum >= super->total_blocks) return NULL;
    return data_blocks + bnum * MEX_BLOCK_SIZE;
}

static struct mex_dirent *dir_find(struct mex_inode *dir, const char *name) {
    if (!dir || !(dir->mode & (MEX_TYPE_DIRECTORY << 16))) return NULL;
    uint64_t offset = 0;
    uint8_t *block = get_block(dir->direct[0]);
    if (!block) return NULL;
    while (offset < dir->size) {
        struct mex_dirent *entry = (struct mex_dirent *)(block + offset);
        if (entry->inode == 0) break;
        if (entry->name_len == strlen(name) && memcmp(entry->name, name, entry->name_len) == 0)
            return entry;
        offset += entry->rec_len;
    }
    return NULL;
}

static int dir_add(struct mex_inode *dir, uint64_t inum, const char *name, uint8_t type) {
    if (!dir || !(dir->mode & (MEX_TYPE_DIRECTORY << 16))) return -1;
    size_t name_len = strlen(name);
    size_t rec_len = ALIGN_UP(sizeof(struct mex_dirent) + name_len, 8);
    if (dir_find(dir, name)) return -1;
    uint64_t offset = 0;
    uint8_t *block = get_block(dir->direct[0]);
    if (!block) {
        uint64_t b = alloc_block();
        if (!b) return -1;
        dir->direct[0] = b;
        dir->blocks = 1;
        block = get_block(b);
        memset(block, 0, MEX_BLOCK_SIZE);
    }
    struct mex_dirent *last = NULL;
    while (offset < dir->size) {
        struct mex_dirent *entry = (struct mex_dirent *)(block + offset);
        if (entry->inode == 0) { last = entry; break; }
        last = entry;
        offset += entry->rec_len;
    }
    if (!last) return -1;
    struct mex_dirent *new_entry;
    if (offset == 0) new_entry = (struct mex_dirent *)block;
    else new_entry = (struct mex_dirent *)((uint8_t *)last + last->rec_len);
    if ((uint8_t *)new_entry + rec_len > block + MEX_BLOCK_SIZE) return -1;
    new_entry->inode = inum;
    new_entry->rec_len = rec_len;
    new_entry->name_len = name_len;
    new_entry->file_type = type;
    memcpy(new_entry->name, name, name_len);
    dir->size += rec_len;
    dir->mtime = timer_get_ticks();
    return 0;
}

static uint64_t path_to_inode(const char *path) {
    if (!path || path[0] != '/') return 0;
    uint64_t current = super->root_inode;
    char component[256];
    const char *p = path + 1;
    while (*p) {
        size_t len = 0;
        while (*p && *p != '/' && len < 255) component[len++] = *p++;
        component[len] = '\0';
        while (*p == '/') p++;
        if (len == 0) break;
        struct mex_inode *dir = get_inode(current);
        if (!dir) return 0;
        struct mex_dirent *entry = dir_find(dir, component);
        if (!entry) return 0;
        current = entry->inode;
    }
    return current;
}

void mexfs_init(void) {
    size_t fs_size = sizeof(struct mex_superblock) + INODE_BITMAP_SIZE + DATA_BITMAP_SIZE + INODE_TABLE_SIZE + DATA_BLOCKS_SIZE;
    fs_base = kzalloc(fs_size);
    if (!fs_base) { vga_puts("FATAL: Cannot allocate filesystem memory\n"); return; }
    super = (struct mex_superblock *)fs_base;
    inode_bitmap = fs_base + sizeof(struct mex_superblock);
    data_bitmap = inode_bitmap + INODE_BITMAP_SIZE;
    inode_table = (struct mex_inode *)(data_bitmap + DATA_BITMAP_SIZE);
    data_blocks = (uint8_t *)inode_table + INODE_TABLE_SIZE;
    mexfs_format("MexaOS");
    mexfs_mount();
    memset(open_files, 0, sizeof(open_files));
    open_file_count = 0;
    next_fid = 1;
}

void mexfs_format(const char *label) {
    memset(fs_base, 0, sizeof(struct mex_superblock) + INODE_BITMAP_SIZE + DATA_BITMAP_SIZE + INODE_TABLE_SIZE);
    super->magic = MEX_MAGIC;
    super->version = MEX_VERSION;
    super->total_blocks = 65536;
    super->free_blocks = 65536 - 1;
    super->total_inodes = MEX_MAX_FILES;
    super->free_inodes = MEX_MAX_FILES - 1;
    super->block_size = MEX_BLOCK_SIZE;
    super->root_inode = 1;
    super->mount_count = 0;
    super->flags = MEXFS_CLEAN | MEXFS_JOURNAL;
    strncpy(super->fs_label, label, 63);
    strncpy(super->os_version, MEXAOS_VERSION, 15);

    struct mex_inode *root = get_inode(1);
    root->mode = (MEX_TYPE_DIRECTORY << 16) | 0755;
    root->direct[0] = alloc_block();
    root->blocks = 1;
    inode_bitmap_set(1);

    uint8_t *root_block = get_block(root->direct[0]);
    struct mex_dirent *dot = (struct mex_dirent *)root_block;
    dot->inode = 1; dot->rec_len = 16; dot->name_len = 1; dot->file_type = MEX_TYPE_DIRECTORY;
    dot->name[0] = '.';
    struct mex_dirent *dotdot = (struct mex_dirent *)(root_block + 16);
    dotdot->inode = 1; dotdot->rec_len = 16; dotdot->name_len = 2; dotdot->file_type = MEX_TYPE_DIRECTORY;
    dotdot->name[0] = '.'; dotdot->name[1] = '.';
    root->size = 32;

    mex_mkdir("/bin", 0755);
    mex_mkdir("/boot", 0755);
    mex_mkdir("/dev", 0755);
    mex_mkdir("/etc", 0755);
    mex_mkdir("/home", 0755);
    mex_mkdir("/lib", 0755);
    mex_mkdir("/mnt", 0755);
    mex_mkdir("/proc", 0755);
    mex_mkdir("/root", 0700);
    mex_mkdir("/system", 0755);
    mex_mkdir("/tmp", 0777);
    mex_mkdir("/usr", 0755);
    mex_mkdir("/var", 0755);
    mex_mkdir("/system/ai", 0755);
    mex_mkdir("/system/intents", 0755);
    mex_mkdir("/system/themes", 0755);
    mex_mkdir("/system/models", 0755);
    mex_mkdir("/usr/apps", 0755);
    mex_mkdir("/usr/share", 0755);

    vga_puts("\n       Filesystem formatted: ");
    vga_print_size((uint64_t)super->total_blocks * MEX_BLOCK_SIZE);
    vga_puts(" total\n");
}

int mexfs_mount(void) {
    if (super->magic != MEX_MAGIC) return -1;
    super->mount_time = 0;
    super->mount_count++;
    super->flags |= MEXFS_DIRTY;
    return 0;
}

void mexfs_unmount(void) {
    super->flags &= ~MEXFS_DIRTY;
    super->flags |= MEXFS_CLEAN;
}

fid_t mex_open(const char *path, uint32_t mode) {
    uint64_t inum = path_to_inode(path);
    if (inum == 0 && (mode & MEX_O_CREATE)) {
        if (mex_create(path, 0644) < 0) return -1;
        inum = path_to_inode(path);
    }
    if (inum == 0) return -1;
    struct mex_inode *inode = get_inode(inum);
    if (!inode) return -1;
    if ((mode & MEX_O_WRITE) && !(inode->mode & 0200)) return -1;
    if ((mode & MEX_O_READ) && !(inode->mode & 0400)) return -1;
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (open_files[i].fid == 0) {
            open_files[i].fid = next_fid++;
            open_files[i].inode_num = inum;
            memcpy(&open_files[i].inode, inode, sizeof(struct mex_inode));
            open_files[i].pos = (mode & MEX_O_APPEND) ? inode->size : 0;
            open_files[i].mode = mode;
            strncpy(open_files[i].path, path, MEX_MAX_PATH - 1);
            inode->atime = 0;
            open_file_count++;
            return open_files[i].fid;
        }
    }
    return -1;
}

int mex_close(fid_t fid) {
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (open_files[i].fid == fid) {
            write_inode(open_files[i].inode_num, &open_files[i].inode);
            open_files[i].fid = 0;
            open_files[i].inode_num = 0;
            open_file_count--;
            return 0;
        }
    }
    return -1;
}

ssize_t mex_read(fid_t fid, void *buf, size_t count) {
    struct mex_file *file = NULL;
    for (int i = 0; i < MAX_OPEN_FILES; i++)
        if (open_files[i].fid == fid) { file = &open_files[i]; break; }
    if (!file || !(file->mode & MEX_O_READ)) return -1;
    size_t to_read = count;
    if (file->pos + to_read > file->inode.size) to_read = file->inode.size - file->pos;
    size_t bytes_read = 0;
    uint8_t *dst = buf;
    while (bytes_read < to_read) {
        uint64_t block_idx = file->pos / MEX_BLOCK_SIZE;
        uint64_t block_offset = file->pos % MEX_BLOCK_SIZE;
        uint64_t block_num;
        if (block_idx < 12) block_num = file->inode.direct[block_idx];
        else break;
        if (!block_num) break;
        uint8_t *block = get_block(block_num);
        if (!block) break;
        size_t chunk = MEX_BLOCK_SIZE - block_offset;
        if (chunk > to_read - bytes_read) chunk = to_read - bytes_read;
        memcpy(dst + bytes_read, block + block_offset, chunk);
        file->pos += chunk;
        bytes_read += chunk;
    }
    return bytes_read;
}

ssize_t mex_write(fid_t fid, const void *buf, size_t count) {
    struct mex_file *file = NULL;
    for (int i = 0; i < MAX_OPEN_FILES; i++)
        if (open_files[i].fid == fid) { file = &open_files[i]; break; }
    if (!file || !(file->mode & MEX_O_WRITE)) return -1;
    size_t bytes_written = 0;
    const uint8_t *src = buf;
    while (bytes_written < count) {
        uint64_t block_idx = file->pos / MEX_BLOCK_SIZE;
        uint64_t block_offset = file->pos % MEX_BLOCK_SIZE;
        uint64_t *block_ptr;
        if (block_idx < 12) block_ptr = &file->inode.direct[block_idx];
        else break;
        if (*block_ptr == 0) {
            *block_ptr = alloc_block();
            if (*block_ptr == 0) break;
            file->inode.blocks++;
        }
        uint8_t *block = get_block(*block_ptr);
        if (!block) break;
        size_t chunk = MEX_BLOCK_SIZE - block_offset;
        if (chunk > count - bytes_written) chunk = count - bytes_written;
        memcpy(block + block_offset, src + bytes_written, chunk);
        file->pos += chunk;
        bytes_written += chunk;
        if (file->pos > file->inode.size) file->inode.size = file->pos;
    }
    file->inode.mtime = 0;
    write_inode(file->inode_num, &file->inode);
    return bytes_written;
}

int mex_seek(fid_t fid, int64_t offset, int whence) {
    struct mex_file *file = NULL;
    for (int i = 0; i < MAX_OPEN_FILES; i++)
        if (open_files[i].fid == fid) { file = &open_files[i]; break; }
    if (!file) return -1;
    switch (whence) {
        case 0: file->pos = offset; break;
        case 1: file->pos += offset; break;
        case 2: file->pos = file->inode.size + offset; break;
        default: return -1;
    }
    if (file->pos < 0) file->pos = 0;
    return 0;
}

int mex_mkdir(const char *path, uint32_t mode) {
    char parent_path[MEX_MAX_PATH];
    char name[256];
    const char *last_slash = strrchr(path, '/');
    if (!last_slash || last_slash == path) {
        strcpy(parent_path, "/");
        strcpy(name, path + 1);
    } else {
        size_t parent_len = last_slash - path;
        if (parent_len == 0) parent_len = 1;
        memcpy(parent_path, path, parent_len);
        parent_path[parent_len] = '\0';
        strcpy(name, last_slash + 1);
    }
    uint64_t parent_inum = path_to_inode(parent_path);
    if (parent_inum == 0) return -1;
    struct mex_inode *parent = get_inode(parent_inum);
    if (!parent) return -1;
    uint64_t new_inum = alloc_inode();
    if (new_inum == 0) return -1;
    struct mex_inode *new_dir = get_inode(new_inum);
    memset(new_dir, 0, sizeof(struct mex_inode));
    new_dir->mode = (MEX_TYPE_DIRECTORY << 16) | (mode & 0xFFFF);
    new_dir->direct[0] = alloc_block();
    new_dir->blocks = 1;
    uint8_t *block = get_block(new_dir->direct[0]);
    memset(block, 0, MEX_BLOCK_SIZE);
    struct mex_dirent *dot = (struct mex_dirent *)block;
    dot->inode = new_inum; dot->rec_len = 16; dot->name_len = 1; dot->file_type = MEX_TYPE_DIRECTORY;
    dot->name[0] = '.';
    struct mex_dirent *dotdot = (struct mex_dirent *)(block + 16);
    dotdot->inode = parent_inum; dotdot->rec_len = 16; dotdot->name_len = 2; dotdot->file_type = MEX_TYPE_DIRECTORY;
    dotdot->name[0] = '.'; dotdot->name[1] = '.';
    new_dir->size = 32;
    dir_add(parent, new_inum, name, MEX_TYPE_DIRECTORY);
    write_inode(parent_inum, parent);
    return 0;
}

int mex_rmdir(const char *path) {
    uint64_t inum = path_to_inode(path);
    if (inum == 0 || inum == 1) return -1;
    struct mex_inode *inode = get_inode(inum);
    if (!inode) return -1;
    inode->dtime = 0;
    inode_bitmap_clear(inum);
    return 0;
}

int mex_create(const char *path, uint32_t mode) {
    char parent_path[MEX_MAX_PATH];
    char name[256];
    const char *last_slash = strrchr(path, '/');
    if (!last_slash) return -1;
    if (last_slash == path) strcpy(parent_path, "/");
    else {
        size_t len = last_slash - path;
        memcpy(parent_path, path, len);
        parent_path[len] = '\0';
    }
    strcpy(name, last_slash + 1);
    uint64_t parent_inum = path_to_inode(parent_path);
    if (parent_inum == 0) return -1;
    struct mex_inode *parent = get_inode(parent_inum);
    if (!parent) return -1;
    uint64_t inum = alloc_inode();
    if (inum == 0) return -1;
    struct mex_inode *inode = get_inode(inum);
    memset(inode, 0, sizeof(struct mex_inode));
    inode->mode = (MEX_TYPE_REGULAR << 16) | (mode & 0xFFFF);
    dir_add(parent, inum, name, MEX_TYPE_REGULAR);
    write_inode(parent_inum, parent);
    return 0;
}

int mex_unlink(const char *path) {
    uint64_t inum = path_to_inode(path);
    if (inum == 0 || inum == 1) return -1;
    struct mex_inode *inode = get_inode(inum);
    if (!inode) return -1;
    for (int i = 0; i < 12; i++)
        if (inode->direct[i]) free_block(inode->direct[i]);
    inode->dtime = 0;
    inode_bitmap_clear(inum);
    return 0;
}

int mex_rename(const char *oldpath, const char *newpath) {
    (void)oldpath; (void)newpath;
    return 0;
}

int mex_stat(const char *path, struct mex_inode *inode) {
    uint64_t inum = path_to_inode(path);
    if (inum == 0) return -1;
    struct mex_inode *src = get_inode(inum);
    if (!src) return -1;
    memcpy(inode, src, sizeof(struct mex_inode));
    return 0;
}

int mex_chmod(const char *path, uint32_t mode) {
    uint64_t inum = path_to_inode(path);
    if (inum == 0) return -1;
    struct mex_inode *inode = get_inode(inum);
    if (!inode) return -1;
    inode->mode = (inode->mode & 0xFFFF0000) | (mode & 0xFFFF);
    write_inode(inum, inode);
    return 0;
}

int mex_set_intent(const char *path, const char *intent) {
    uint64_t inum = path_to_inode(path);
    if (inum == 0) return -1;
    struct mex_inode *inode = get_inode(inum);
    if (!inode) return -1;
    strncpy(inode->intent_tag, intent, 127);
    inode->intent_tag[127] = '\0';
    inode->mtime = 0;
    write_inode(inum, inode);
    return 0;
}

int mex_get_intent(const char *path, char *intent, size_t size) {
    uint64_t inum = path_to_inode(path);
    if (inum == 0) return -1;
    struct mex_inode *inode = get_inode(inum);
    if (!inode) return -1;
    strncpy(intent, inode->intent_tag, size - 1);
    intent[size - 1] = '\0';
    return 0;
}

int mex_execute_intent(const char *intent_text) {
    return ai_process_intent(intent_text);
}

ssize_t mex_read_mex_file(const char *path, void **data, size_t *size) {
    fid_t fid = mex_open(path, MEX_O_READ);
    if (fid < 0) return -1;
    struct mex_file *file = NULL;
    for (int i = 0; i < MAX_OPEN_FILES; i++)
        if (open_files[i].fid == fid) { file = &open_files[i]; break; }
    if (!file) { mex_close(fid); return -1; }
    *size = file->inode.size;
    *data = kmalloc(*size);
    if (!*data) { mex_close(fid); return -1; }
    ssize_t ret = mex_read(fid, *data, *size);
    mex_close(fid);
    return ret;
}

ssize_t mex_write_mex_file(const char *path, const void *data, size_t size, const char *intent) {
    fid_t fid = mex_open(path, MEX_O_WRITE | MEX_O_CREATE | MEX_O_TRUNC);
    if (fid < 0) return -1;
    ssize_t written = mex_write(fid, data, size);
    if (intent && intent[0]) {
        for (int i = 0; i < MAX_OPEN_FILES; i++)
            if (open_files[i].fid == fid) {
                strncpy(open_files[i].inode.intent_tag, intent, 127);
                write_inode(open_files[i].inode_num, &open_files[i].inode);
                break;
            }
    }
    mex_close(fid);
    return written;
}

int mex_validate_mex(const char *path) {
    uint64_t inum = path_to_inode(path);
    if (inum == 0) return -1;
    struct mex_inode *inode = get_inode(inum);
    if (!inode) return -1;
    return (inode->intent_tag[0] != '\0') ? 1 : 0;
}

void mexfs_dump_info(void) {
    vga_puts("\n  MexaOS File System (.mex)\n");
    vga_puts("  ══════════════════════════\n");
    vga_puts("  Label:      "); vga_puts(super->fs_label); vga_puts("\n");
    vga_puts("  Version:    "); vga_puts(super->os_version); vga_puts("\n");
    vga_puts("  Block size: "); vga_print_dec(super->block_size); vga_puts(" bytes\n");
    vga_puts("  Total:      "); vga_print_dec(super->total_blocks); vga_puts(" blocks (");
    vga_print_size(super->total_blocks * super->block_size); vga_puts(")\n");
    vga_puts("  Free:       "); vga_print_dec(super->free_blocks); vga_puts(" blocks (");
    vga_print_size(super->free_blocks * super->block_size); vga_puts(")\n");
    vga_puts("  Used:       "); vga_print_dec(super->total_blocks - super->free_blocks);
    vga_puts(" blocks\n");
    vga_puts("  Inodes:     "); vga_print_dec(super->total_inodes - super->free_inodes);
    vga_puts(" / "); vga_print_dec(super->total_inodes); vga_puts("\n");
    vga_puts("  Flags:      ");
    if (super->flags & MEXFS_CLEAN) vga_puts("CLEAN ");
    if (super->flags & MEXFS_DIRTY) vga_puts("DIRTY ");
    if (super->flags & MEXFS_READONLY) vga_puts("RO ");
    if (super->flags & MEXFS_JOURNAL) vga_puts("JOURNAL ");
    if (super->flags & MEXFS_ENCRYPTED) vga_puts("ENCRYPTED ");
    if (super->flags & MEXFS_COMPRESSED) vga_puts("COMPRESSED ");
    vga_puts("\n");
    vga_puts("  Open files: "); vga_print_dec(open_file_count); vga_puts("\n");
}

int mexfs_check(void) {
    if (super->magic != MEX_MAGIC) return -1;
    if (super->version != MEX_VERSION) return -2;
    if (super->free_blocks > super->total_blocks) return -3;
    if (super->free_inodes > super->total_inodes) return -4;
    return 0;
}

uint64_t mexfs_free_space(void) { return super->free_blocks * super->block_size; }
uint64_t mexfs_total_space(void) { return super->total_blocks * super->block_size; }

static char *strrchr(const char *s, int c) {
    const char *last = NULL;
    while (*s) { if (*s == c) last = s; s++; }
    return (char *)last;
}

static size_t strlen(const char *s) { size_t len = 0; while (s[len]) len++; return len; }

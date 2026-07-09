/* ============================================================
 *  MexaOS Filesystem (MexFS)
 * ============================================================ */

#ifndef MEXAOS_MEXFS_H
#define MEXAOS_MEXFS_H

#include <stdint.h>
#include <stddef.h>

/* ─── Type Definitions ─── */
typedef int fid_t;
typedef long ssize_t;

/* ─── Constants ─── */
#define MEX_MAX_PATH        256
#define MEX_MAX_NAME        128
#define MEX_MAX_FILES       1024
#define MEX_BLOCK_SIZE      4096
#define MEX_MAGIC           0x4D455846  /* "MEXF" */

/* ─── File Types ─── */
#define MEX_TYPE_REGULAR    0
#define MEX_TYPE_DIRECTORY  1
#define MEX_TYPE_SYMLINK    2
#define MEX_TYPE_DEVICE     3
#define MEX_TYPE_PIPE       4

/* ─── Open Modes ─── */
#define MEX_MODE_READ       0x01
#define MEX_MODE_WRITE      0x02
#define MEX_MODE_CREATE     0x04
#define MEX_MODE_APPEND     0x08
#define MEX_MODE_TRUNCATE   0x10

/* ─── MexFS Superblock ─── */
struct mexfs_superblock {
    uint32_t magic;
    uint32_t version;
    uint64_t total_blocks;
    uint64_t free_blocks;
    uint64_t total_inodes;
    uint64_t free_inodes;
    uint64_t root_inode;
};

/* ─── MexFS Inode ─── */
struct mexfs_inode {
    uint64_t inode_num;
    uint32_t type;
    uint32_t mode;
    uint64_t size;
    uint64_t blocks;
    uint64_t created;
    uint64_t modified;
    uint64_t accessed;
    uint32_t link_count;
    uint64_t data_blocks[12];
    uint64_t indirect_block;
    uint64_t double_indirect;
};

/* ─── File Handle ─── */
struct mexfs_file {
    fid_t fid;
    uint64_t inode_num;
    uint32_t mode;
    uint64_t position;
    char path[MEX_MAX_PATH];
};

/* ─── Filesystem Functions ─── */
void mexfs_init(void);
void mexfs_mount(void);
void mexfs_unmount(void);

/* ─── File Operations ─── */
fid_t mex_open(const char *path, uint32_t mode);
int mex_close(fid_t fid);
ssize_t mex_read(fid_t fid, void *buf, size_t count);
ssize_t mex_write(fid_t fid, const void *buf, size_t count);
int mex_seek(fid_t fid, int64_t offset, int whence);
int mex_truncate(fid_t fid, uint64_t size);

/* ─── Directory Operations ─── */
int mex_mkdir(const char *path);
int mex_rmdir(const char *path);
int mex_chdir(const char *path);
int mex_getcwd(char *buf, size_t size);

/* ─── File System Info ─── */
int mex_stat(const char *path, struct mexfs_inode *stat);
int mex_exists(const char *path);
int mex_unlink(const char *path);
int mex_rename(const char *oldpath, const char *newpath);

/* ─── Intent-Aware Files ─── */
ssize_t mex_read_mex_file(const char *path, void **data, size_t *size);
ssize_t mex_write_mex_file(const char *path, const void *data, size_t size, const char *intent);

#endif

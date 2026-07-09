/* ============================================================
 *  MexaOS File System Header (.mex)
 *  The intent-aware native file system
 * ============================================================ */

#ifndef MEXFS_H
#define MEXFS_H

#include "../../include/mexaos.h"

/* ─── Superblock ─── */
struct mex_superblock {
    uint32_t magic;             /* MEX_MAGIC */
    uint32_t version;           /* MEX_VERSION */
    uint64_t total_blocks;      /* Total data blocks */
    uint64_t free_blocks;       /* Free data blocks */
    uint64_t total_inodes;      /* Total inodes */
    uint64_t free_inodes;       /* Free inodes */
    uint64_t block_size;        /* Block size (4096) */
    uint64_t inode_table_start; /* Block of inode table */
    uint64_t data_bitmap_start; /* Block of data bitmap */
    uint64_t inode_bitmap_start;/* Block of inode bitmap */
    uint64_t data_start;        /* First data block */
    uint64_t root_inode;        /* Root directory inode */
    uint64_t journal_start;     /* Journal blocks */
    uint64_t journal_size;      /* Journal size in blocks */
    
    /* MexaOS-specific */
    char fs_label[64];          /* Volume label */
    char os_version[16];        /* Created by MexaOS version */
    uint64_t creation_time;     /* FS creation timestamp */
    uint64_t mount_time;        /* Last mount time */
    uint64_t write_time;        /* Last write time */
    uint64_t mount_count;       /* Mount count */
    uint32_t flags;             /* FS flags */
    uint32_t reserved;          /* Padding */
    
    uint64_t checksum;          /* Superblock checksum */
} __attribute__((packed));

/* FS flags */
#define MEXFS_CLEAN     0x01
#define MEXFS_DIRTY     0x02
#define MEXFS_READONLY  0x04
#define MEXFS_JOURNAL   0x08
#define MEXFS_ENCRYPTED 0x10
#define MEXFS_COMPRESSED 0x20

/* ─── Inode ─── */
struct mex_inode {
    uint32_t mode;              /* File type and permissions */
    uint32_t uid;               /* Owner ID */
    uint32_t gid;               /* Group ID */
    uint64_t size;              /* File size in bytes */
    uint64_t blocks;            /* Blocks allocated */
    uint64_t atime;             /* Access time */
    uint64_t mtime;             /* Modification time */
    uint64_t ctime;             /* Creation time */
    uint64_t dtime;             /* Deletion time */
    
    /* Direct blocks (12 x 4KB = 48KB direct) */
    uint64_t direct[12];
    /* Single indirect (1 x 4KB = 512 x 4KB = 2MB) */
    uint64_t indirect;
    /* Double indirect (512 x 512 x 4KB = 1GB) */
    uint64_t double_indirect;
    /* Triple indirect */
    uint64_t triple_indirect;
    
    /* MexaOS: Intent metadata */
    char intent_tag[128];       /* Associated intent */
    char ai_summary[256];       /* AI-generated summary */
    uint32_t ai_confidence;     /* AI processing confidence 0-100 */
    uint32_t access_count;      /* Number of accesses */
    uint32_t importance_score;  /* AI-calculated importance */
    
    uint64_t reserved[4];
    uint64_t checksum;
} __attribute__((packed));

/* ─── Directory Entry ─── */
struct mex_dirent {
    uint64_t inode;             /* Inode number */
    uint16_t rec_len;           /* Record length */
    uint8_t  name_len;          /* Name length */
    uint8_t  file_type;         /* File type */
    char     name[];            /* Variable-length name */
} __attribute__((packed));

/* ─── File Handle ─── */
struct mex_file {
    fid_t fid;                  /* File ID */
    uint64_t inode_num;         /* Inode number */
    struct mex_inode inode;     /* Cached inode */
    uint64_t pos;               /* Current position */
    uint32_t mode;              /* Open mode */
    char path[MEX_MAX_PATH];    /* Full path */
    
    /* Linked list for open files */
    struct mex_file *next;
};

/* ─── VFS Operations ─── */
void mexfs_init(void);
void mexfs_format(const char *label);
int mexfs_mount(void);
void mexfs_unmount(void);

/* File operations */
fid_t mex_open(const char *path, uint32_t mode);
int mex_close(fid_t fid);
ssize_t mex_read(fid_t fid, void *buf, size_t count);
ssize_t mex_write(fid_t fid, const void *buf, size_t count);
int mex_seek(fid_t fid, int64_t offset, int whence);
int mex_truncate(fid_t fid, uint64_t size);

/* Directory operations */
int mex_mkdir(const char *path, uint32_t mode);
int mex_rmdir(const char *path);
int mex_readdir(const char *path, struct mex_dirent **entries, size_t *count);

/* File management */
int mex_create(const char *path, uint32_t mode);
int mex_unlink(const char *path);
int mex_rename(const char *oldpath, const char *newpath);
int mex_stat(const char *path, struct mex_inode *inode);
int mex_chmod(const char *path, uint32_t mode);

/* MexaOS: Intent operations */
int mex_set_intent(const char *path, const char *intent);
int mex_get_intent(const char *path, char *intent, size_t size);
int mex_execute_intent(const char *intent_text);

/* .mex file format */
ssize_t mex_read_mex_file(const char *path, void **data, size_t *size);
ssize_t mex_write_mex_file(const char *path, const void *data, size_t size, const char *intent);
int mex_validate_mex(const char *path);

/* Utility */
void mexfs_dump_info(void);
int mexfs_check(void);
uint64_t mexfs_free_space(void);
uint64_t mexfs_total_space(void);

#endif /* MEXFS_H */

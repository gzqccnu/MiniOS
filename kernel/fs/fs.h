/*
 * MiniOS
 * Copyright (C) 2025 lrisguan <lrisguan@outlook.com>
 *
 * This program is released under the terms of the GNU General Public License version 2(GPLv2).
 * See https://opensource.org/licenses/GPL-2.0 for more information.
 *
 * Project homepage: https://github.com/lrisguan/MiniOS
 * Description: A scratch implemention of OS based on RISC-V
 */

// fs.h - simple inode-based filesystem interface

#ifndef _FS_H_
#define _FS_H_

#include <stdint.h>

// maximum number of simultaneously open file descriptors managed by filesystem
// file descriptors returned to user space start from FS_FD_BASE (so 0,1,2 can
// be reserved for stdin/stdout/stderr)
#define FS_MAX_FILES 16
#define FS_FD_BASE 3
// maximum filename length (including terminating '\0')
#define FS_NAME_MAX 16

typedef struct {
  int used;
  uint32_t inum;   // inode number on disk
  uint32_t offset; // current read/write offset
} FSFileDesc;

// On-disk layout (very small, for 64KB disk.img):
// block 0: superblock
// blocks [1..4]: inode table
// block 5: free block bitmap
// blocks [6..127]: data blocks

#define BSIZE 512
#define FSS_MAGIC 0x4d4f5346U /* "FSOM" arbitrary magic */

#define N_BLOCKS 128
#define NINODE 32

#define SB_BLOCK 0
#define INODE_START_BLOCK 1
#define INODE_BLOCKS 4
#define BITMAP_BLOCK (INODE_START_BLOCK + INODE_BLOCKS)
#define DATA_START_BLOCK (BITMAP_BLOCK + 1)

#define NDIRECT 10

// on-disk superblock
struct superblock {
  uint32_t magic;
  uint32_t nblocks;
  uint32_t ninodes;
  uint32_t root_inum; // inode number of root directory
};

// on-disk inode
struct dinode {
  uint16_t type;           // 0: free, 1: file, 2: directory
  uint16_t nlink;          // not really used yet
  uint32_t size;           // size in bytes
  uint32_t addrs[NDIRECT]; // direct data blocks
};

struct dirent {
  uint32_t inum;
  char name[FS_NAME_MAX];
};

static struct superblock sb;
static FSFileDesc fs_fds[FS_MAX_FILES];

void fs_init(void);
int fs_create(const char *name);
int fs_open(const char *name);
int fs_read(int fd, void *buf, int n);
int fs_write(int fd, const void *buf, int n);
int fs_close(int fd);

// remove a file from root directory (unlink)
int fs_unlink(const char *name);

// truncate file (set size to 0) by name in root directory
int fs_trunc(const char *name);

// list entries in the root directory; return number of entries, or -1 on error
int fs_list_root(struct dirent *ents, int max_ents);

#endif

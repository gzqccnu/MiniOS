/*
 * Lrix
 * Copyright (C) 2025 lrisguan <lrisguan@outlook.com>
 *
 * This program is released under the terms of the GNU General Public License version 2(GPLv2).
 * See https://opensource.org/licenses/GPL-2.0 for more information.
 *
 * Project homepage: https://github.com/lrisguan/Lrix
 * Description: A scratch implemention of OS based on RISC-V
 */

// fs.c - simple inode-based filesystem on top of virtio-blk

#include "fs.h"
#include "../include/log.h"
#include "../string/string.h"
#include "blk.h"

static int b_read(uint32_t blockno, void *buf) {
  if (blockno >= N_BLOCKS)
    return -1;
#ifdef FS_DEBUG
  printk(BLUE "[INFO]: \tfs: b_read block %lu" RESET "\n", blockno);
#endif
  int r = blk_read_sector(blockno, buf);
#ifdef FS_DEBUG
  printk(BLUE "[INFO]: \tfs: b_read done, ret=%d" RESET "\n", r);
#endif
  return r;
}

static int b_write(uint32_t blockno, const void *buf) {
  if (blockno >= N_BLOCKS)
    return -1;
#ifdef FS_DEBUG
  printk(BLUE "[INFO]: \tfs: b_write block %lu" RESET "\n", blockno);
#endif
  int r = blk_write_sector(blockno, buf);
#ifdef FS_DEBUG
  printk(BLUE "[INFO]: \tfs: b_write done, ret=%d" RESET "\n", r);
#endif
  return r;
}

// free a previously allocated data block in bitmap
static int b_free(uint32_t blockno) {
  if (blockno < DATA_START_BLOCK || blockno >= N_BLOCKS)
    return -1;
  char buf[BSIZE];
  if (b_read(BITMAP_BLOCK, buf) < 0)
    return -1;
  uint32_t bi = blockno - DATA_START_BLOCK;
  uint32_t byte = bi / 8;
  uint8_t mask = (uint8_t)(1u << (bi % 8));
  buf[byte] &= (uint8_t)~mask;
  if (b_write(BITMAP_BLOCK, buf) < 0)
    return -1;
  return 0;
}

static int namecmp(const char *a, const char *b) {
  for (int i = 0; i < FS_NAME_MAX; i++) {
    char ca = a[i];
    char cb = b[i];
    if (ca != cb)
      return (int)(unsigned char)ca - (int)(unsigned char)cb;
    if (ca == '\0')
      break;
  }
  return 0;
}

static int read_dinode(uint32_t inum, struct dinode *dip) {
  if (inum == 0 || inum >= NINODE)
    return -1;
  uint32_t idx = inum - 1;
  uint32_t block = INODE_START_BLOCK + (idx * sizeof(struct dinode)) / BSIZE;
  uint32_t off = (idx * sizeof(struct dinode)) % BSIZE;
  char buf[BSIZE];
  if (b_read(block, buf) < 0)
    return -1;
  memcpy(dip, buf + off, sizeof(struct dinode));
  return 0;
}

static int write_dinode(uint32_t inum, const struct dinode *dip) {
  if (inum == 0 || inum >= NINODE)
    return -1;
  uint32_t idx = inum - 1;
  uint32_t block = INODE_START_BLOCK + (idx * sizeof(struct dinode)) / BSIZE;
  uint32_t off = (idx * sizeof(struct dinode)) % BSIZE;
  char buf[BSIZE];
  if (b_read(block, buf) < 0)
    return -1;
  memcpy(buf + off, dip, sizeof(struct dinode));
  if (b_write(block, buf) < 0)
    return -1;
  return 0;
}

static int balloc(uint32_t *out_blockno) {
  char buf[BSIZE];
  if (b_read(BITMAP_BLOCK, buf) < 0)
    return -1;
  for (uint32_t b = DATA_START_BLOCK; b < N_BLOCKS; b++) {
    uint32_t bi = b - DATA_START_BLOCK;
    uint32_t byte = bi / 8;
    uint8_t mask = (uint8_t)(1u << (bi % 8));
    if (!(buf[byte] & mask)) {
      buf[byte] |= mask;
      if (b_write(BITMAP_BLOCK, buf) < 0)
        return -1;
      *out_blockno = b;
      return 0;
    }
  }
  return -1;
}

static uint32_t bmap(struct dinode *dip, uint32_t file_block_idx, int alloc) {
  // First, handle direct blocks
  if (file_block_idx < NDIRECT) {
    uint32_t bno = dip->addrs[file_block_idx];
    if (bno == 0 && alloc) {
      if (balloc(&bno) < 0)
        return 0;
      dip->addrs[file_block_idx] = bno;
    }
    return bno;
  }

  // then handle single-level indirect block
  uint32_t idx = file_block_idx - NDIRECT;
  if (idx >= NINDIRECT)
    return 0;

  uint32_t indirect_bno = dip->indirect;
  char buf[BSIZE];
  uint32_t *a = (uint32_t *)buf;

  // If necessary, first allocate a data block for the indirect block itself and
  // initialize it to zero
  if (indirect_bno == 0) {
    if (!alloc)
      return 0;
    if (balloc(&indirect_bno) < 0)
      return 0;
    memset(buf, 0, BSIZE);
    if (b_write(indirect_bno, buf) < 0)
      return 0;
    dip->indirect = indirect_bno;
  } else {
    if (b_read(indirect_bno, buf) < 0)
      return 0;
  }

  uint32_t bno = a[idx];
  if (bno == 0 && alloc) {
    if (balloc(&bno) < 0)
      return 0;
    a[idx] = bno;
    if (b_write(indirect_bno, buf) < 0)
      return 0;
  }
  return bno;
}

static int inode_read(uint32_t inum, void *dst, uint32_t off, uint32_t n) {
  struct dinode din;
  if (read_dinode(inum, &din) < 0)
    return -1;
  if (off >= din.size)
    return 0;
  if (off + n > din.size)
    n = din.size - off;

  uint32_t tot = 0;
  char buf[BSIZE];
  while (tot < n) {
    uint32_t fblk = (off + tot) / BSIZE;
    uint32_t boff = (off + tot) % BSIZE;
    uint32_t bno = bmap(&din, fblk, 0);
    if (bno == 0)
      break;
    if (b_read(bno, buf) < 0)
      return -1;
    uint32_t remain_block = BSIZE - boff;
    uint32_t remain_req = n - tot;
    uint32_t m = remain_block < remain_req ? remain_block : remain_req;
    memcpy((char *)dst + tot, buf + boff, m);
    tot += m;
  }
  return (int)tot;
}

static int inode_write(uint32_t inum, const void *src, uint32_t off, uint32_t n) {
  struct dinode din;
  if (read_dinode(inum, &din) < 0)
    return -1;

  uint32_t tot = 0;
  char buf[BSIZE];
  while (tot < (uint32_t)n) {
    uint32_t fblk = (off + tot) / BSIZE;
    uint32_t boff = (off + tot) % BSIZE;
    uint32_t bno = bmap(&din, fblk, 1);
    if (bno == 0)
      return -1;
    if (b_read(bno, buf) < 0)
      return -1;
    uint32_t remain_block = BSIZE - boff;
    uint32_t remain_req = (uint32_t)n - tot;
    uint32_t m = remain_block < remain_req ? remain_block : remain_req;
    memcpy(buf + boff, (const char *)src + tot, m);
    if (b_write(bno, buf) < 0)
      return -1;
    tot += m;
  }
  if (off + tot > din.size)
    din.size = off + tot;
  if (write_dinode(inum, &din) < 0)
    return -1;
  return (int)tot;
}

static int ialloc(uint16_t type, uint32_t *out_inum) {
  struct dinode din;
  for (uint32_t inum = 1; inum < NINODE; inum++) {
    if (read_dinode(inum, &din) < 0)
      return -1;
    if (din.type == 0) {
      memset(&din, 0, sizeof(din));
      din.type = type;
      din.nlink = 1;
      din.size = 0;
      if (write_dinode(inum, &din) < 0)
        return -1;
      *out_inum = inum;
      return 0;
    }
  }
  return -1;
}

static int dir_lookup(const char *name, uint32_t *out_inum) {
  uint32_t inum = sb.root_inum;
  struct dinode din;
  if (read_dinode(inum, &din) < 0)
    return -1;
  uint32_t off = 0;
  struct dirent de;
  while (off + sizeof(de) <= din.size) {
    int r = inode_read(inum, &de, off, sizeof(de));
    if (r != (int)sizeof(de))
      return -1;
    if (de.inum != 0 && namecmp(de.name, name) == 0) {
      *out_inum = de.inum;
      return 0;
    }
    off += sizeof(de);
  }
  return -1;
}

static int dir_add(const char *name, uint32_t inum) {
  uint32_t root = sb.root_inum;
  struct dinode din;
  if (read_dinode(root, &din) < 0)
    return -1;
  struct dirent de;
  memset(&de, 0, sizeof(de));
  de.inum = inum;
  int i = 0;
  while (i < FS_NAME_MAX - 1 && name[i]) {
    de.name[i] = name[i];
    i++;
  }
  de.name[i] = '\0';
  if (inode_write(root, &de, din.size, sizeof(de)) != (int)sizeof(de))
    return -1;
  return 0;
}

// remove directory entry with given name from root directory
/* remove directory entry with given inode number from root directory */
static int dir_remove_inum(uint32_t target_inum) {
  uint32_t root = sb.root_inum;
  struct dinode din;
  if (read_dinode(root, &din) < 0)
    return -1;

  uint32_t off = 0;
  struct dirent de;
  while (off + sizeof(de) <= din.size) {
    int r = inode_read(root, &de, off, sizeof(de));
    if (r != (int)sizeof(de))
      return -1;
    if (de.inum == target_inum && de.inum != 0) {
      // clear this entry
      de.inum = 0;
      de.name[0] = '\0';
      if (inode_write(root, &de, off, sizeof(de)) != (int)sizeof(de))
        return -1;
      return 0;
    }
    off += sizeof(de);
  }
  return -1;
}

static void fs_format(void) {
  char buf[BSIZE];

  INFO("fs: formatting disk image");

  // zero inode blocks and bitmap
  memset(buf, 0, BSIZE);
  for (uint32_t b = INODE_START_BLOCK; b < INODE_START_BLOCK + INODE_BLOCKS; b++)
    b_write(b, buf);
  b_write(BITMAP_BLOCK, buf);

  // init superblock
  sb.magic = FSS_MAGIC;
  sb.nblocks = N_BLOCKS;
  sb.ninodes = NINODE;
  sb.root_inum = 1;
  memset(buf, 0, BSIZE);
  memcpy(buf, &sb, sizeof(sb));
  b_write(SB_BLOCK, buf);

  // allocate root directory inode
  struct dinode din;
  memset(&din, 0, sizeof(din));
  din.type = 2; // directory
  din.nlink = 1;
  din.size = 0;
  write_dinode(sb.root_inum, &din);

  // Create a README file in the root directory and write the content generated
  // from the top-level README.md during build
  // The current filesystem supports NDIRECT direct blocks + NINDIRECT indirect
  // blocks, for a total of MAXFILE data blocks.
  if (README_MD_SIZE > 0) {
    uint32_t readme_inum;
    if (ialloc(1, &readme_inum) == 0) {
      if (dir_add("README.md", readme_inum) == 0) {
        uint32_t to_write = README_MD_SIZE;
        uint32_t max_size = MAXFILE * BSIZE;
        if (to_write > max_size)
          to_write = max_size;
        // Write the README content from memory to the new inode
        (void)inode_write(readme_inum, README_MD, 0, to_write);
      }
    }
  }
}

void fs_init(void) {
  INFO("fs: init start");
  // reset fd table
  for (int i = 0; i < FS_MAX_FILES; i++) {
    fs_fds[i].used = 0;
    fs_fds[i].inum = 0;
    fs_fds[i].offset = 0;
  }

  char buf[BSIZE];
  if (b_read(SB_BLOCK, buf) < 0) {
    INFO("fs: no superblock, format new fs");
    fs_format();
    return;
  }
  memcpy(&sb, buf, sizeof(sb));
  if (sb.magic != FSS_MAGIC) {
    INFO("fs: bad magic, format new fs");
    fs_format();
  } else {
    printk(BLUE "[INFO]: \tfs: superblock loaded, magic=%x" RESET "\n", sb.magic);
  }
}

static int fs_alloc_fd(uint32_t inum) {
  for (int i = 0; i < FS_MAX_FILES; i++) {
    if (!fs_fds[i].used) {
      fs_fds[i].used = 1;
      fs_fds[i].inum = inum;
      fs_fds[i].offset = 0;
      // external-facing fd is offset by FS_FD_BASE so that 0,1,2
      // can be reserved for stdin/stdout/stderr
      return FS_FD_BASE + i;
    }
  }
  return -1;
}

int fs_create(const char *name) {
  if (!name)
    return -1;
  uint32_t existing;
  if (dir_lookup(name, &existing) == 0)
    return -1;

  uint32_t inum;
  if (ialloc(1, &inum) < 0)
    return -1;
  if (dir_add(name, inum) < 0)
    return -1;
  return fs_alloc_fd(inum);
}

int fs_open(const char *name) {
  if (!name)
    return -1;
  uint32_t inum;
  if (dir_lookup(name, &inum) < 0)
    return -1;
  return fs_alloc_fd(inum);
}

int fs_read(int fd, void *buf, int n) {
  if (fd < FS_FD_BASE || fd >= FS_FD_BASE + FS_MAX_FILES || !buf || n < 0)
    return -1;
  FSFileDesc *d = &fs_fds[fd - FS_FD_BASE];
  if (!d->used)
    return -1;
  int r = inode_read(d->inum, buf, d->offset, (uint32_t)n);
  if (r > 0)
    d->offset += (uint32_t)r;
  return r;
}

int fs_write(int fd, const void *buf, int n) {
  if (fd < FS_FD_BASE || fd >= FS_FD_BASE + FS_MAX_FILES || !buf || n < 0)
    return -1;
  FSFileDesc *d = &fs_fds[fd - FS_FD_BASE];
  if (!d->used)
    return -1;
  int r = inode_write(d->inum, buf, d->offset, (uint32_t)n);
  if (r > 0)
    d->offset += (uint32_t)r;
  return r;
}

int fs_close(int fd) {
  if (fd < FS_FD_BASE || fd >= FS_FD_BASE + FS_MAX_FILES)
    return -1;
  FSFileDesc *d = &fs_fds[fd - FS_FD_BASE];
  if (!d->used)
    return -1;
  d->used = 0;
  d->inum = 0;
  d->offset = 0;
  return 0;
}

// unlink a file in root directory: remove dirent and free its inode data blocks
int fs_unlink(const char *name) {
  if (!name)
    return -1;

  // first locate inode by name using same logic as fs_open/fs_create
  uint32_t inum = 0;
  if (dir_lookup(name, &inum) < 0)
    return -1;

  if (inum == 0)
    return -1;

  struct dinode din;
  if (read_dinode(inum, &din) < 0)
    return -1;

  // free all direct data blocks
  for (int i = 0; i < NDIRECT; i++) {
    uint32_t bno = din.addrs[i];
    if (bno != 0) {
      b_free(bno);
      din.addrs[i] = 0;
    }
  }
  // free indirect blocks and the data blocks they point to
  if (din.indirect != 0) {
    char buf[BSIZE];
    if (b_read(din.indirect, buf) == 0) {
      uint32_t *a = (uint32_t *)buf;
      for (uint32_t i = 0; i < NINDIRECT; i++) {
        if (a[i] != 0)
          b_free(a[i]);
      }
    }
    b_free(din.indirect);
    din.indirect = 0;
  }
  din.size = 0;
  din.type = 0; // mark inode as free
  din.nlink = 0;
  if (write_dinode(inum, &din) < 0)
    return -1;

  // finally remove the directory entry pointing to this inode
  if (dir_remove_inum(inum) < 0)
    return -1;

  return 0;
}

// truncate file (set size to 0) without freeing data blocks (they will be reused on next
// writes); only visible size is affected.
int fs_trunc(const char *name) {
  if (!name)
    return -1;

  uint32_t inum = 0;
  if (dir_lookup(name, &inum) < 0)
    return -1;
  if (inum == 0)
    return -1;

  struct dinode din;
  if (read_dinode(inum, &din) < 0)
    return -1;

  din.size = 0;
  if (write_dinode(inum, &din) < 0)
    return -1;

  return 0;
}

// enumerate entries in the root directory
int fs_list_root(struct dirent *ents, int max_ents) {
  if (!ents || max_ents <= 0)
    return -1;

  uint32_t root = sb.root_inum;
  struct dinode din;
  if (read_dinode(root, &din) < 0)
    return -1;

  uint32_t off = 0;
  struct dirent de;
  int count = 0;
  while (off + sizeof(de) <= din.size && count < max_ents) {
    int r = inode_read(root, &de, off, sizeof(de));
    if (r != (int)sizeof(de))
      return -1;
    if (de.inum != 0 && de.name[0] != '\0') {
      ents[count] = de;
      count++;
    }
    off += sizeof(de);
  }
  return count;
}

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

#include "syscall.h"
#include "../fs/fs.h"
#include "../mem/kmem.h"
#include "../mem/vmm.h"
#include "../proc/proc.h"
#include "../uart/uart.h"
#include <stdint.h>

/* CLINT base used for uptime */
#define CLINT_BASE 0x02000000UL
#define CLINT_MTIME (CLINT_BASE + 0xBFF8)

/* Simple syscall implementations */
static uint64_t sys_getpid(uint64_t args[6], uint64_t epc) {
  PCB *p = get_current_proc();
  if (!p)
    return 0;
  return (uint64_t)p->pid;
}

static uint64_t sys_exit(uint64_t args[6], uint64_t epc) {
  (void)args;
  (void)epc;
  proc_exit();
  return 0; // not reached
}

static uint64_t sys_uptime(uint64_t args[6], uint64_t epc) {
  (void)args;
  (void)epc;
  volatile uint64_t *mtime = (uint64_t *)CLINT_MTIME;
  return *mtime;
}

static uint64_t sys_sleep(uint64_t args[6], uint64_t epc) {
  uint64_t ticks = args[0];
  (void)epc;
  volatile uint64_t *mtime = (uint64_t *)CLINT_MTIME;
  uint64_t start = *mtime;
  while ((*mtime - start) < ticks) {
    asm volatile("wfi");
  }
  return 0;
}

static uint64_t sys_write(uint64_t args[6], uint64_t epc) {
  uint64_t fd = args[0];
  const char *buf = (const char *)args[1];
  uint64_t len = args[2];
  (void)epc;
  if (fd == 1 || fd == 2) {
    for (uint64_t i = 0; i < len; i++) {
      char c = buf[i];
      printk("%c", c);
    }
    return len;
  }
  // filesystem-backed fds live in [FS_FD_BASE, FS_FD_BASE + FS_MAX_FILES)
  if (fd >= FS_FD_BASE && fd < FS_FD_BASE + FS_MAX_FILES)
    return (uint64_t)fs_write((int)fd, buf, (int)len);
  return (uint64_t)-1;
}

static uint64_t sys_open(uint64_t args[6], uint64_t epc) {
  (void)epc;
  const char *name = (const char *)args[0];
  int create = (int)args[1];
  int fd;
  if (create)
    fd = fs_create(name);
  else
    fd = fs_open(name);
  return (uint64_t)fd;
}

static uint64_t sys_read(uint64_t args[6], uint64_t epc) {
  (void)epc;
  int fd = (int)args[0];
  void *buf = (void *)args[1];
  int n = (int)args[2];
  if (fd >= FS_FD_BASE && fd < FS_FD_BASE + FS_MAX_FILES)
    return (uint64_t)fs_read(fd, buf, n);
  return (uint64_t)-1;
}

static uint64_t sys_close(uint64_t args[6], uint64_t epc) {
  (void)epc;
  int fd = (int)args[0];
  if (fd >= FS_FD_BASE && fd < FS_FD_BASE + FS_MAX_FILES)
    return (uint64_t)fs_close(fd);
  return (uint64_t)-1;
}

// blocking read single char from UART console
static uint64_t sys_getc(uint64_t args[6], uint64_t epc) {
  (void)args;
  (void)epc;
  char c = uart_getc_blocking();
  return (uint64_t)(unsigned char)c;
}

// unlink file in root directory
static uint64_t sys_unlink(uint64_t args[6], uint64_t epc) {
  (void)epc;
  const char *name = (const char *)args[0];
  if (!name)
    return (uint64_t)-1;
  int r = fs_unlink(name);
  return (uint64_t)r;
}

static uint64_t sys_trunc(uint64_t args[6], uint64_t epc) {
  (void)epc;
  const char *name = (const char *)args[0];
  if (!name)
    return (uint64_t)-1;
  int r = fs_trunc(name);
  return (uint64_t)r;
}

// simple ps: dump process list to console
static uint64_t sys_ps(uint64_t args[6], uint64_t epc) {
  (void)args;
  (void)epc;
  proc_dump();
  return 0;
}

// list entries in root directory; args[0]=buffer, args[1]=max entries
static uint64_t sys_ls(uint64_t args[6], uint64_t epc) {
  (void)epc;
  struct dirent *ents = (struct dirent *)args[0];
  int max_ents = (int)args[1];
  if (!ents || max_ents <= 0)
    return (uint64_t)-1;
  int n = fs_list_root(ents, max_ents);
  return (uint64_t)n;
}
static uint64_t sys_fork(uint64_t args[6], uint64_t epc) {
  (void)args;
  PCB *child = proc_fork(epc);
  if (!child)
    return (uint64_t)-1;
  return (uint64_t)child->pid;
}

static uint64_t sys_wait(uint64_t args[6], uint64_t epc) {
  (void)args;
  (void)epc;
  int pid = proc_wait_and_reap();
  return (uint64_t)pid;
}

/* User heap virtual layout:
 * Each process gets a per-pid heap region starting at HEAP_USER_BASE + pid * PER_PROC_HEAP.
 * We map physical pages into that virtual region using vmm_map_page so the virtual
 * addresses are contiguous per-process.
 */
#define HEAP_USER_BASE 0x80400000UL
#define PER_PROC_HEAP (8 * 1024) /* 8KB per process */

static uint64_t sys_sbrk(uint64_t args[6], uint64_t epc) {
  (void)epc;
  uint64_t incr = args[0];
  PCB *p = get_current_proc();
  if (!p)
    return (uint64_t)-1;

  /* initialize brk_base if not set: allocate a per-process virtual region base */
  if (!p->brk_base) {
    uintptr_t base = HEAP_USER_BASE + (uintptr_t)p->pid * PER_PROC_HEAP;
    p->brk_base = (void *)base;
    p->brk_size = 0;
  }

  uint64_t old_brk = (uint64_t)p->brk_base + p->brk_size;
  if (incr == 0)
    return old_brk;

  uint64_t need_pages = (incr + PAGE_SIZE - 1) / PAGE_SIZE;
  for (uint64_t i = 0; i < need_pages; i++) {
    void *vaddr = (void *)((uint8_t *)p->brk_base + p->brk_size + i * PAGE_SIZE);
    /* map a new physical page to this virtual address */
    if (vmm_map_page(vaddr, VMM_P_RW | VMM_P_USER) != 0) {
      return (uint64_t)-1; /* mapping failed */
    }
  }
  p->brk_size += need_pages * PAGE_SIZE;
  return old_brk;
}

// ==== exec support: map program name to linked-in user entry point ====

typedef void (*exec_entry_fn)(int, char **);

typedef struct ExecEntry {
  const char *name;
  exec_entry_fn entry;
} ExecEntry;

// declare user-space entry functions that are linked into the kernel image
extern void user_shell(void);
extern void hello_main(int argc, char **argv);

static ExecEntry exec_table[] = {
    {"sh", (exec_entry_fn)user_shell},
    {"hello", hello_main},
};

static int exec_table_count = sizeof(exec_table) / sizeof(exec_table[0]);

// simple strcmp implementation (to avoid depending on kernel string.h symbols here)
static int kstrcmp(const char *a, const char *b) {
  while (*a && *a == *b) {
    a++;
    b++;
  }
  return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

// look up program name (args[0]) in exec_table and return entry address, or -1
uint64_t sys_exec_lookup(uint64_t args[6]) {
  const char *name = (const char *)args[0];
  if (!name)
    return (uint64_t)-1;

  for (int i = 0; i < exec_table_count; i++) {
    if (kstrcmp(exec_table[i].name, name) == 0) {
      return (uint64_t)exec_table[i].entry;
    }
  }
  return (uint64_t)-1;
}

uint64_t syscall_dispatch(uint64_t num, uint64_t args[6], uint64_t epc) {
  switch (num) {
  case SYS_GETPID:
    return sys_getpid(args, epc);
  case SYS_EXIT:
    return sys_exit(args, epc);
  case SYS_UPTIME:
    return sys_uptime(args, epc);
  case SYS_SLEEP:
    return sys_sleep(args, epc);
  case SYS_WRITE:
    return sys_write(args, epc);
  case SYS_OPEN:
    return sys_open(args, epc);
  case SYS_READ:
    return sys_read(args, epc);
  case SYS_CLOSE:
    return sys_close(args, epc);
  case SYS_FORK:
    return sys_fork(args, epc);
  case SYS_WAIT:
    return sys_wait(args, epc);
  case SYS_SBRK:
    return sys_sbrk(args, epc);
  case SYS_LS:
    return sys_ls(args, epc);
  case SYS_GETC:
    return sys_getc(args, epc);
  case SYS_UNLINK:
    return sys_unlink(args, epc);
  case SYS_TRUNC:
    return sys_trunc(args, epc);
  case SYS_PS:
    return sys_ps(args, epc);
  // SYS_EXEC is handled specially in trap.c so that it can change mepc/arguments; do not
  // process it here.
  default:
    /* unsupported syscall */
    return (uint64_t)-1;
  }
}

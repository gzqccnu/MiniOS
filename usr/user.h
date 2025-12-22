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

// user-space common header for MiniOS (syscall wrappers)

#ifndef _USER_H_
#define _USER_H_

#include "../kernel/fs/fs.h"
#include "../kernel/string/string.h"
#include "../kernel/syscall/syscall.h"
#include <stdint.h>

// generic syscall helper (defined in sys_call.c)
uint64_t sys_call3(uint64_t num, uint64_t a0, uint64_t a1, uint64_t a2);

// syscall wrappers
void sys_exit(int code);
int sys_getpid(void);
long sys_sleep(unsigned long ticks);
long sys_write(int fd, const void *buf, uint64_t len);
int sys_open(const char *name, int create);
long sys_read(int fd, void *buf, uint64_t len);
int sys_close(int fd);
int sys_ls(struct dirent *ents, int max_ents);
int sys_getc(void);
int sys_unlink(const char *name);
int sys_fork(void);
int sys_wait(void);

// exec: replace current process with named program (does not return on success)
int sys_exec(const char *name);

// truncate file by name (size -> 0)
int sys_trunc(const char *name);

// list processes (ps)
int sys_ps(void);

#endif /* _USER_H_ */

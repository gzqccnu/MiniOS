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

#include "user.h"

long sys_write(int fd, const void *buf, uint64_t len) {
  return (long)sys_call3(SYS_WRITE, (uint64_t)fd, (uint64_t)buf, len);
}

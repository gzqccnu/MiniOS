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

#include "user.h"

int sys_open(const char *name, int create) {
  return (int)sys_call3(SYS_OPEN, (uint64_t)name, (uint64_t)create, 0);
}

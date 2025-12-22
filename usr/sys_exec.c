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

// simplified exec: on success, does not return (process image replaced)
int sys_exec(const char *name) { return (int)sys_call3(SYS_EXEC, (uint64_t)name, 0, 0); }

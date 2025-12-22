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

int sys_ps(void) { return (int)sys_call3(SYS_PS, 0, 0, 0); }

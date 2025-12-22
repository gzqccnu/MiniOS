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

#ifndef _TRAP_H_
#define _TRAP_H_

#include <stdint.h>

/* Control trap printing: set to 1 for debug (verbose) mode, 0 for silent mode.
 * Can be overridden by -DTRAP_DEBUG=0 in CFLAGS.
 */
#ifndef TRAP_DEBUG
#define TRAP_DEBUG 0
#endif

// unsigned long read_csr(const char *name);
void trap_init(void);
void trap_handler_c(uint64_t *tf);

#endif /* _TRAP_H_ */

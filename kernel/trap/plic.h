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

#ifndef _PLIC_H_
#define _PLIC_H_
#include <stdint.h>
// PLIC base address for QEMU virt machine
#define PLIC_BASE 0x0c000000L
#define PLIC_PRIORITY (PLIC_BASE + 0x0)
#define PLIC_PENDING (PLIC_BASE + 0x1000)
#define PLIC_ENABLE (PLIC_BASE + 0x2000)
#define PLIC_THRESHOLD(hart) (PLIC_BASE + 0x200000 + (hart)*0x1000)
#define PLIC_CLAIM(hart) (PLIC_BASE + 0x200004 + (hart)*0x1000)

void plic_init(void);
uint32_t plic_claim(void);
void plic_complete(uint32_t irq);

#endif /* _PLIC_H_ */

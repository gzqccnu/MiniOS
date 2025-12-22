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

// types.h

#ifndef _TYPES_H_
#define _TYPES_H_

#include <stdint.h>

// standard macro defination
#define NULL ((void *)0)
#define va_list __builtin_va_list
#define va_start __builtin_va_start
#define va_end __builtin_va_end
#define va_arg __builtin_va_arg

// RISC-V64 register context(the registers need to save)
typedef struct RegisterState {
  uint64_t x1;      // ra - return address
  uint64_t x5;      // t0
  uint64_t x6;      // t1
  uint64_t x7;      // t2
  uint64_t x8;      // s0/fp
  uint64_t x9;      // s1
  uint64_t x10;     // a0
  uint64_t x11;     // a1
  uint64_t x12;     // a2
  uint64_t x13;     // a3
  uint64_t x14;     // a4
  uint64_t x15;     // a5
  uint64_t x16;     // a6
  uint64_t x17;     // a7
  uint64_t x18;     // s2
  uint64_t x19;     // s3
  uint64_t x20;     // s4
  uint64_t x21;     // s5
  uint64_t x22;     // s6
  uint64_t x23;     // s7
  uint64_t x24;     // s8
  uint64_t x25;     // s9
  uint64_t x26;     // s10
  uint64_t x27;     // s11
  uint64_t x28;     // t3
  uint64_t x29;     // t4
  uint64_t x30;     // t5
  uint64_t x31;     // t6
  uint64_t sepc;    // exception return address
  uint64_t sp;      // stack pointer
  uint64_t mstatus; // mstatus
} RegState;

#endif

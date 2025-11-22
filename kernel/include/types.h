// types.h

#ifndef _TYPES_H_
#define _TYPES_H_

typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long uint64;

// typedef uint64 pde_t;

// 标准宏定义
#define NULL ((void *)0)
#define va_list __builtin_va_list
#define va_start __builtin_va_start
#define va_end __builtin_va_end
#define va_arg __builtin_va_arg

// RISC-V64寄存器上下文（需要保存的寄存器）
typedef struct RegisterState {
  uint64 x1;   // ra - 返回地址
  uint64 x5;   // t0
  uint64 x6;   // t1
  uint64 x7;   // t2
  uint64 x8;   // s0/fp
  uint64 x9;   // s1
  uint64 x10;  // a0
  uint64 x11;  // a1
  uint64 x12;  // a2
  uint64 x13;  // a3
  uint64 x14;  // a4
  uint64 x15;  // a5
  uint64 x16;  // a6
  uint64 x17;  // a7
  uint64 x18;  // s2
  uint64 x19;  // s3
  uint64 x20;  // s4
  uint64 x21;  // s5
  uint64 x22;  // s6
  uint64 x23;  // s7
  uint64 x24;  // s8
  uint64 x25;  // s9
  uint64 x26;  // s10
  uint64 x27;  // s11
  uint64 x28;  // t3
  uint64 x29;  // t4
  uint64 x30;  // t5
  uint64 x31;  // t6
  uint64 sepc; // 异常返回地址
  uint64 sp;   // 栈指针
} RegState;

#endif

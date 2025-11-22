#include "trap.h"
#include "../include/log.h"
#include "../uart/uart.h"
#include <stdint.h>

/* 声明由汇编提供的入口 */
extern void trap_vector_entry(void);

/* Helper: 读取 CSR */
// static inline unsigned long read_csr(const char *name) {
//     unsigned long val = 0;
//     if (name == (const char *)"mcause") {
//         asm volatile("csrr %0, mcause" : "=r"(val));
//     } else if (name == (const char *)"mepc") {
//         asm volatile("csrr %0, mepc" : "=r"(val));
//     } else if (name == (const char *)"mtval") {
//         asm volatile("csrr %0, mtval" : "=r"(val));
//     } else if (name == (const char *)"mstatus") {
//         asm volatile("csrr %0, mstatus" : "=r"(val));
//     } else {
//         val = 0;
//     }
//     return val;
// }

// /* 设置 mtvec 指向 trap_vector_entry（直接模式） */
// void trap_init(void) {
//     uintptr_t vec = (uintptr_t)trap_vector_entry;
//     asm volatile("csrw mtvec, %0" :: "r"(vec));
//     printk("[trap]: \tmtvec set to %p\n", (void*)vec);
// }

// /* C-level trap handler called from assembly entry */
// void trap_handler_c(void) {
//     unsigned long cause, epc, tval, mstatus;
//     asm volatile("csrr %0, mcause" : "=r"(cause));
//     asm volatile("csrr %0, mepc" : "=r"(epc));
//     asm volatile("csrr %0, mtval" : "=r"(tval));
//     asm volatile("csrr %0, mstatus" : "=r"(mstatus));

//     /* Print values */
//     printk("\n[trap]: \tTRAP! mcause=0x%lx mepc=0x%lx mtval=0x%lx mstatus=0x%lx\n",
//            cause, epc, tval, mstatus);

//     /* Decode a few common causes (machine mode) */
//     unsigned long code = cause & 0xFFF;      /* low bits hold exception code */
//     if ((cause >> (sizeof(unsigned long)*8 - 1)) & 1) {
//         /* interrupt */
//         printk("[trap]: \tinterrupt (code=%lu)\n", code);
//     } else {
//         printk("[trap]: \texception (code=%lu)\n", code);
//         switch (code) {
//             case 0:  printk("[trap]: \tinstr addr misaligned\n"); break;
//             case 1:  printk("[trap]: \tinstr access fault\n"); break;
//             case 2:  printk("[trap]: \tillegal instruction\n"); break;
//             case 3:  printk("[trap]: \tbreakpoint\n"); break;
//             case 4:  printk("[trap]: \tload address misaligned\n"); break;
//             case 5:  printk("[trap]: \tload access fault\n"); break;
//             case 6:  printk("[trap]: \tstore/AMO address misaligned\n"); break;
//             case 7:  printk("[trap]: \tstore/AMO access fault\n"); break;
//             case 8:  printk("[trap]: \tenv call from U-mode\n"); break;
//             case 9:  printk("[trap]: \tenv call from S-mode\n"); break;
//             case 11: printk("[trap]: \tenv call from M-mode\n"); break;
//             case 12: printk("[trap]: \tinstruction page fault\n"); break;  /* instruction page
//             fault */ case 13: printk("[trap]: \tload page fault\n"); break;         /* load page
//             fault */ case 15: printk("[trap]: \tstore/AMO page fault\n"); break;    /* store/AMO
//             page fault */ default: printk("[trap]: \tunknown exception code=%lu\n", code); break;
//         }
//     }

//     /* optionally spin so we can inspect */
//     while (1) { asm volatile("wfi"); }
// }
extern void trap_vector_entry(void);

/* 内联函数：读取RISC-V CSR寄存器（类型安全，避免字符串比较） */
static inline uint64_t read_mcause(void) {
  uint64_t val;
  asm volatile("csrr %0, mcause" : "=r"(val));
  return val;
}

static inline uint64_t read_mepc(void) {
  uint64_t val;
  asm volatile("csrr %0, mepc" : "=r"(val));
  return val;
}

static inline uint64_t read_mtval(void) {
  uint64_t val;
  asm volatile("csrr %0, mtval" : "=r"(val));
  return val;
}

static inline uint64_t read_mstatus(void) {
  uint64_t val;
  asm volatile("csrr %0, mstatus" : "=r"(val));
  return val;
}

/* 设置 mtvec 指向 trap_vector_entry（直接模式，确保4字节对齐） */
void trap_init(void) {
  // 确保基地址4字节对齐（直接模式要求低2位为0）
  uintptr_t vec = (uintptr_t)trap_vector_entry & ~0x3UL;
  asm volatile("csrw mtvec, %0" ::"r"(vec));
  printk(MAGENTA "[trap]: \tmtvec initialized to 0x%lx (direct mode)\n" RESET, vec);
}

/* C-level trap handler：解析并打印trap信息（调试用） */
void trap_handler_c(void) {
  uint64_t cause = read_mcause();
  uint64_t epc = read_mepc();
  uint64_t tval = read_mtval();
  uint64_t mstatus = read_mstatus();

  // 解析中断/异常标志（mcause最高位：1=中断，0=异常）
  int is_interrupt = (cause >> (sizeof(uint64_t) * 8 - 1)) & 1;
  // 解析完整代码（清除最高位标志）
  uint64_t code = cause & ~(1UL << (sizeof(uint64_t) * 8 - 1));

  // 打印基本信息
  printk(RED "[trap]: \t==== TRAP OCCURRED ====\n" RESET);
  printk(RED "[trap]: \ttype: %s (code=0x%lx)\n" RESET, is_interrupt ? "interrupt" : "exception",
         code);
  printk(RED "[trap]: \tmepc: 0x%lx (instruction address when trap occurred)\n" RESET, epc);
  printk(
      RED
      "[trap]: \tmtval: 0x%lx (exception-related value (e.g., fault address/instruction))\n" RESET,
      tval);
  printk(RED "[trap]: \tmstatus: 0x%lx (status register)\n" RESET, mstatus);
  // Detailed exception type parsing (RISC-V standard exception codes)

  // 详细解析异常类型（RISC-V标准异常代码）
  if (!is_interrupt) {
    printk(RED "[trap]: \texception detail: " RESET);
    switch (code) {
    case 0:
      printk(RED "instruction address misaligned\n" RESET);
      break;
    case 1:
      printk(RED "instruction access fault\n" RESET);
      break;
    case 2:
      printk(RED "illegal instruction\n" RESET);
      break;
    case 3:
      printk(RED "breakpoint (triggered by ebreak instruction)\n" RESET);
      break;
    case 4:
      printk(RED "load address misaligned\n" RESET);
      break;
    case 5:
      printk(RED "load access fault\n" RESET);
      break;
    case 6:
      printk(RED "store/AMO address misaligned\n" RESET);
      break;
    case 7:
      printk(RED "store/AMO access fault\n" RESET);
      break;
    case 8:
      printk(RED "environment call from U-mode\n" RESET);
      break;
    case 9:
      printk(RED "environment call from S-mode\n" RESET);
      break;
    case 11:
      printk(RED "environment call from M-mode\n" RESET);
      break;
    case 12:
      printk(RED "instruction page fault\n" RESET);
      break;
    case 13:
      printk(RED "load page fault\n" RESET);
      break;
    case 15:
      printk(RED "store/AMO page fault\n" RESET);
      break;
    default:
      printk(RED "unknown exception (code=0x%lx)\n" RESET, code);
      break;
    }
  } else {
    printk(RED "[trap]: \tinterrupt detail: " RESET);
    switch (code) {
    case 3:
      printk(RED "machine software interrupt\n" RESET);
      break;
    case 7:
      printk(RED "machine timer interrupt\n" RESET);
      break;
    case 11:
      printk(RED "machine external interrupt\n" RESET);
      break;
    default:
      printk(RED "unknown interrupt, code=0x%lx\n" RESET, code);
      break;
    }
  }
  // 调试用：陷入后暂停（避免反复触发）
  printk(RED "[trap]: \tentering infinite loop...\n" RESET);
  while (1) {
    asm volatile("wfi"); // 等待中断（降低CPU占用）
  }
}
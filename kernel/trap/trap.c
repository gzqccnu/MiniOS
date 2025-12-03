#include "trap.h"
#include "../include/log.h"
#include "../uart/uart.h"
#include <stdint.h>

/* Declare an entry provided by assembly */
extern void trap_vector_entry(void);

/* forward scheduler */
extern void schedule(void);

/* CLINT (QEMU virt) addresses for machine timer */
#define CLINT_BASE 0x02000000UL
#define CLINT_MTIME (CLINT_BASE + 0xBFF8)
#define CLINT_MTIMECMP(hartid) (CLINT_BASE + 0x4000 + 8 * (hartid))

static void set_next_timer(uint64_t interval) {
  volatile uint64_t *mtime = (uint64_t *)CLINT_MTIME;
  volatile uint64_t *mtimecmp = (uint64_t *)CLINT_MTIMECMP(0);
  uint64_t now = *mtime;
  *mtimecmp = now + interval;
}

extern void trap_vector_entry(void);

/* Inline function: read RISC-V CSR register (type-safe, avoids string comparison) */
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

/* Set mtvec to point to trap_vector_entry (direct mode, ensure 4-byte alignment) */
void trap_init(void) {
  /* Ensure the base address is 4-byte aligned
   * (direct mode requires the lowest 2 bits to be 0)
   */
  uintptr_t vec = (uintptr_t)trap_vector_entry & ~0x3UL;
  asm volatile("csrw mtvec, %0" ::"r"(vec));
  printk(MAGENTA "[trap]: \tmtvec initialized to 0x%x (direct mode)\n" RESET, vec);

  /* enable machine-timer interrupt in MIE and global MIE in mstatus */
  const unsigned long MTIE = (1UL << 7);
  const unsigned long MIE_BIT = (1UL << 3);
  asm volatile("csrs mie, %0" ::"r"(MTIE));
  asm volatile("csrs mstatus, %0" ::"r"(MIE_BIT));

  /* program first timer (small interval) */
  set_next_timer(1000000ULL);
}

/* C-level trap handler：parse and print trap info (debug) */
void trap_handler_c(void) {
  uint64_t cause = read_mcause();
  uint64_t epc = read_mepc();
  uint64_t tval = read_mtval();
  uint64_t mstatus = read_mstatus();

  /* Interrupt/Exception Cause Flag
   * (mcause most significant bit: 1 = interrupt, 0 = exception)
   */
  int is_interrupt = (cause >> (sizeof(uint64_t) * 8 - 1)) & 1;
  // Parse complete code (clear the highest bit flag)
  uint64_t code = cause & ~(1UL << (sizeof(uint64_t) * 8 - 1));

  // print basic info
  printk(RED "[trap]: \t==== TRAP OCCURRED ====\n" RESET);
  printk(RED "[trap]: \ttype: %s (code=0x%x)\n" RESET, is_interrupt ? "interrupt" : "exception",
         code);
  printk(RED "[trap]: \tmepc: 0x%x (instruction address when trap occurred)\n" RESET, epc);
  printk(
      RED
      "[trap]: \tmtval: 0x%x (exception-related value (e.g., fault address/instruction))\n" RESET,
      tval);
  printk(RED "[trap]: \tmstatus: 0x%x (status register)\n" RESET, mstatus);
  // Detailed exception type parsing (RISC-V standard exception codes)

  // Detailed analysis of exception types (RISC-V standard exception codes)
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
      printk(RED "unknown exception (code=0x%x)\n" RESET, code);
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
      /* reprogram the timer for the next tick */
      set_next_timer(1000000ULL);
      /* invoke scheduler to perform context switch */
      schedule();
      return; /* after schedule and switch, return to trap entry which will mret */
      break;
    case 11:
      printk(RED "machine external interrupt\n" RESET);
      break;
    default:
      printk(RED "unknown interrupt, code=0x%x\n" RESET, code);
      break;
    }
  }
  // Debugging: pause after getting stuck (to avoid repeated triggers)调试用：陷入后暂停（避免反复触发）
  printk(RED "[trap]: \tentering infinite loop...\n" RESET);
  while (1) {
    asm volatile("wfi"); // Waiting for interrupt (reduces CPU usage)
  }
}

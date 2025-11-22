#ifndef _RISCV_H_
#define _RISCV_H_
#include <stdint.h>

#define SSTATUS_SIE (1L << 1)

static inline uint64_t csrr_sstatus() {
  uint64_t x;
  asm volatile("csrr %0, sstatus" : "=r"(x));
  return x;
}

static inline void csrw_sstatus(uint64_t x) { asm volatile("csrw sstatus, %0" : : "r"(x)); }

// enable device interrupts
static inline void intr_on() { csrw_sstatus(csrr_sstatus() | SSTATUS_SIE); }

// disable device interrupts
static inline void intr_off() { csrw_sstatus(csrr_sstatus() & ~SSTATUS_SIE); }

#endif /* _RISCV_H_ */

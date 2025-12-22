#ifndef _RISCV_H_
#define _RISCV_H_
#include <stdint.h>

#define MSTATUS_SIE (1UL << 3)

static inline uint64_t csrr_mstatus() {
  uint64_t x;
  asm volatile("csrr %0, mstatus" : "=r"(x));
  return x;
}

static inline void csrw_mstatus(uint64_t x) { asm volatile("csrw mstatus, %0" : : "r"(x)); }

static inline void intr_on() {
  unsigned long x = 1UL << 3; // MIE bit
  asm volatile("csrs mstatus, %0" ::"r"(x));
}

static inline void intr_off() {
  unsigned long x = 1UL << 3; // MIE bit
  asm volatile("csrc mstatus, %0" ::"r"(x));
}
#endif /* _RISCV_H_ */

#include "user.h"

uint64_t sys_call3(uint64_t num, uint64_t a0, uint64_t a1, uint64_t a2) {
  register uint64_t r0 asm("a0") = a0;
  register uint64_t r1 asm("a1") = a1;
  register uint64_t r2 asm("a2") = a2;
  register uint64_t r7 asm("a7") = num;
  asm volatile("ecall" : "+r"(r0) : "r"(r1), "r"(r2), "r"(r7) : "memory");
  return r0;
}

#include "plic.h"
#include "../include/log.h"
#include "../uart/uart.h"

void plic_init(void) {
  int hart = 0;

  // On QEMU virt, VirtIO devices are usually mapped to IRQ 1 ~ 8
  // Enable all of them to avoid missing a disk on IRQ 2 or IRQ 3 due to probe order
  for (int irq = 1; irq <= 8; irq++) {
    // 1. Set priority (Priority > 0 means enabled)
    *(uint32_t *)(PLIC_PRIORITY + irq * 4) = 1;

    // 2. Enable interrupts
    // Enable register is a bitmap: Bit 1 for IRQ 1, Bit 2 for IRQ 2, etc.
    *(uint32_t *)(PLIC_ENABLE + hart * 0x80) |= (1 << irq);
  }

  // 3. Set threshold = 0 (allow all interrupts with priority > 0)
  *(uint32_t *)PLIC_THRESHOLD(hart) = 0;

  printk(BLUE "[INFO]: \tplic init done, enabled IRQs 1-8" RESET "\n");
}

// Helper: tell PLIC we are claiming an interrupt (start handling)
uint32_t plic_claim(void) {
  int hart = 0;
  return *(uint32_t *)PLIC_CLAIM(hart);
}

// Helper: tell PLIC we have completed handling an interrupt
void plic_complete(uint32_t irq) {
  int hart = 0;
  *(uint32_t *)PLIC_CLAIM(hart) = irq;
}

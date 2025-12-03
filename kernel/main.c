// main.c - kernel main
#include "include/log.h"
#include "mem/kmem.h"
#include "mem/vmm.h" // virtual memory manager interface
#include "proc/proc.h"
#include "trap/trap.h" // interrupt and exception handling
#include "uart/uart.h" // declarations for uart_init and printk

extern char _heap_start[]; // from linker.ld define where the heap starts
extern char _heap_end[];   // from linker.ld define where the heap ends

// kernel main function
int kmain() {

  uart_init();                   // UART initialization for serial output
  kinit(_heap_start, _heap_end); // initialize kernel memory manager
  vmm_init();                    // initialize virtual memory
  scheduler_init();              // initialize process scheduler

  INFO("welcome to MiniOS!");

  /***** Simple process tests (create two kernel threads) *****/
  INFO("=== Process tests: creating two kernel threads ===");

  /* declare test functions implemented below */
  extern void proc_fn_a(void);
  extern void proc_fn_b(void);

  INFO("=== Starting scheduler (timer interrupts will preempt) ===");
  PCB *p1 = proc_create("procA", (uint64_t)proc_fn_a, 0);
  PCB *p2 = proc_create("procB", (uint64_t)proc_fn_b, 0);

  if (!p1 || !p2) {
    ERROR("Failed to create processes");
    while (1)
      ;
  }
  SUCCESS("Processes created. Starting scheduler...");

  /* let the kernel idle; timer interrupts will invoke scheduler */
  trap_init(); // trap/interrupt initialization

  while (1) {
    asm volatile("wfi");
  }

  return 0;
}

void proc_fn_a(void) {
  int i;
  printk("\n[procA] I am a temporary worker. I will exit soon.\n");
  for (i = 0; i < 5; i++) {
    printk("[procA] working %d/5...\n", i + 1);
    // simple delay
    for (volatile int j = 0; j < 5000000; j++)
      ;
  }

  printk("[procA] Work done! Calling proc_exit()...\n");

  // exit
  proc_exit();

  // this will never be executed
  printk("[procA] ERROR: I am a zombie!\n");
  while (1)
    ;
}

void proc_fn_b(void) {
  printk("[procB] \thello from B\n");
  // delay loop
  for (volatile int i = 0; i < 1000000; i++)
    ;
  printk("[procB] Work done! Calling proc_exit()...\n");
  proc_exit();
}

// main.c - kernel main

#include "include/log.h"
#include "mem/kmem.h"  // kernel memory manager interface
#include "trap/trap.h" // interrupt and exception handling
#include "uart/uart.h" // declarations for uart_init and printk

extern char _heap_start[]; // from linker.ld define where the heap starts
extern char _heap_end[];   // from linker.ld define where the heap ends

// kernel main function
int kmain() {

  trap_init();                   // trap/interrupt initialization
  uart_init();                   // UART initialization for serial output
  kinit(_heap_start, _heap_end); // initialize kernel memory manager
  INFO("welcome to MiniOS!");

  printk("free pages: %d\n", get_free_pages());
  printk("total pages: %d\n", get_total_pages());
  int *ptr = kalloc();
  printk("Allocated page at address: 0x%x\n", ptr);
  print_memory_stats();
  printk("free pages: %d\n", get_free_pages());
  printk("total pages: %d\n", get_total_pages());
  kfree((void *)ptr);
  printk("Freed page at address: 0x%x\n", ptr);
  print_memory_stats();
  printk("free pages: %d\n", get_free_pages());
  printk("total pages: %d\n", get_total_pages());
  return 0;
}

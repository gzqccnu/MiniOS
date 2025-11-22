// main.c - kernel main

#include "include/log.h"
#include "mem/kmem.h"  // kernel memory manager interface
#include "mem/vmm.h"   // virtual memory manager interface
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

  INFO("=== Initializing VMM ===");
  vmm_init();
  EXPECT(vmm_get_page_directory() != NULL, "page directory created");

  INFO("=== VMM TEST 1: basic map/translate/unmap (no hw paging) ===");
  void *vaddr = (void *)0x40000000; // 任意虚拟地址（只是逻辑地址）
  printk("Mapping virtual address %p ...\n", vaddr);

  int r = vmm_map_page(vaddr, VMM_P_RW);
  EXPECT(r == 0, "vmm_map_page returns 0");

  void *pa = vmm_translate(vaddr); // 逻辑映射表查询 -> 得到实际物理页地址
  EXPECT(pa != NULL, "vmm_translate returns physical address");

  printk("vaddr=%p maps to physical %p\n", vaddr, pa);

  // 在未启用分页时：直接通过物理地址写以验证物理页确实存在并为零化
  uint32_t *phys_u32 = (uint32_t *)pa;
  *phys_u32 = 0xCAFEBABE;
  EXPECT(*phys_u32 == 0xCAFEBABE, "write via physical addr -> physical page updated");

  // 解除映射并释放物理页
  r = vmm_unmap(vaddr, 1);
  EXPECT(r == 0, "vmm_unmap works");
  EXPECT(vmm_translate(vaddr) == NULL, "vaddr unmapped");

  INFO("=== VMM TEST 2: multi-page mapping (no hw paging) ===");
  const int N = 4;
  void *vaddrs[N];
  void *paddrs[N];

  for (int i = 0; i < N; i++) {
    vaddrs[i] = (void *)(0x50000000 + i * PAGE_SIZE);
    r = vmm_map_page(vaddrs[i], VMM_P_RW);
    EXPECT(r == 0, "vmm_map_page multi");

    paddrs[i] = vmm_translate(vaddrs[i]);
    EXPECT(paddrs[i] != NULL, "multi translate != NULL");
  }

  // 通过物理地址写入，验证互不干扰
  for (int i = 0; i < N; i++) {
    uint32_t *pp = (uint32_t *)paddrs[i];
    *pp = 0x12340000 + i;
  }
  for (int i = 0; i < N; i++) {
    uint32_t *pp = (uint32_t *)paddrs[i];
    EXPECT(*pp == 0x12340000 + i, "multi write affects correct physical page");
  }

  // cleanup: 解除并释放物理页
  for (int i = 0; i < N; i++) {
    vmm_unmap(vaddrs[i], 1);
  }

  INFO("=== VMM tests finished ===");

  return 0;
}

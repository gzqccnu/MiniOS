/*
 * Lrix
 * Copyright (C) 2025 lrisguan <lrisguan@outlook.com>
 * 
 * This program is released under the terms of the GNU General Public License version 2(GPLv2).
 * See https://opensource.org/licenses/GPL-2.0 for more information.
 * 
 * Project homepage: https://github.com/lrisguan/Lrix
 * Description: A scratch implemention of OS based on RISC-V
 */

#include "vmm.h"
#include "../include/log.h"
#include "../string/string.h"

#define ENTRIES_PER_TABLE 1024
#define PDE_INDEX(addr) (((uint32_t)(addr) >> 22) & 0x3FF)
#define PTE_INDEX(addr) (((uint32_t)(addr) >> 12) & 0x3FF)
#define PAGE_OFFSET(addr) ((uint32_t)(addr)&0xFFF)

/* Virtual address of the kernel page address (accessible by the kernel) */
static vmm_pde_t *kernel_pd = NULL;
/* Physical address of the kernel page address */
static uint32_t kernel_pd_phys = 0;

static void page_zero(void *p) {
  uint8_t *b = (uint8_t *)p;
  for (size_t i = 0; i < VMM_PAGE_SIZE; ++i)
    b[i] = 0;
}

/* set CR3 to pd_phys */
extern void arch_set_cr3(uint32_t pd_phys);
/* set CR0 PG "bit to enable paging */
extern void arch_enable_paging(void);

void arch_set_cr3(uint32_t pd_phys) { (void)pd_phys; }
void arch_enable_paging(void) {}

/* Return a newly allocated and zeroed page (as a page table or page directory page) */
static void *alloc_page_table_page(void) {
  void *p = kalloc();
  if (!p)
    return NULL;
  page_zero(p);
  return p;
}

/* Package the physical address into a PDE/PTE value (lower 12 bits are flags) */
static inline vmm_pde_t make_entry(uint32_t paddr, uint32_t flags) {
  return (vmm_pde_t)((paddr & 0xFFFFF000u) | (flags & 0xFFFu));
}

/* Get the physical address from a page table (virtual address pointer): */
static uint32_t virt_to_phys(void *v) { return (uint32_t)v; }

/* Get or create a page table: if it does not exist
 * allocate a page and insert it into the page directory
 *
 */
static vmm_pte_t *get_or_create_pte_table(uint32_t vaddr, int *created) {
  uint32_t pde_idx = PDE_INDEX(vaddr);
  vmm_pde_t pde = kernel_pd[pde_idx];
  if ((pde & VMM_P_PRESENT) == 0) {
    /* Allocate a new page table page */
    void *pt_page = alloc_page_table_page();
    if (!pt_page)
      return NULL;
    uint32_t pt_phys = virt_to_phys(pt_page);
    kernel_pd[pde_idx] = make_entry(pt_phys, VMM_P_PRESENT | VMM_P_RW | VMM_P_USER);
    if (created)
      *created = 1;
    return (vmm_pte_t *)pt_page;
  } else {
    if (created)
      *created = 0;
    uint32_t pt_phys = pde & 0xFFFFF000u;
    return (vmm_pte_t *)(uintptr_t)pt_phys;
  }
}

/* Get an existing page table (do not create)
 * return NULL if it does not exist
 */
static vmm_pte_t *get_pte_table(uint32_t vaddr) {
  uint32_t pde_idx = PDE_INDEX(vaddr);
  vmm_pde_t pde = kernel_pd[pde_idx];
  if ((pde & VMM_P_PRESENT) == 0)
    return NULL;
  uint32_t pt_phys = pde & 0xFFFFF000u;
  return (vmm_pte_t *)(uintptr_t)pt_phys;
}

/* Initialize VMM: allocate and zero out the kernel page directory */
void vmm_init(void) {
  INFO("vmm: initialize");
  if (kernel_pd)
    return; // already initialized

  /* allocate kernel page directory */
  void *pd_page = alloc_page_table_page();
  if (!pd_page) {
    ERROR("vmm: failed to allocate page directory");
    return;
  }
  kernel_pd = (vmm_pde_t *)pd_page;
  kernel_pd_phys = virt_to_phys(pd_page);

  printk(BLUE "[INFO]: \tvmm: page directory created at virt=%p phys=0x%x\n", kernel_pd,
         kernel_pd_phys);
}

/* Return the virtual address of the current page directory */
vmm_pde_t *vmm_get_page_directory(void) { return kernel_pd; }
void vmm_set_page_directory(vmm_pde_t *pd) {
  kernel_pd = pd;
  kernel_pd_phys = virt_to_phys(pd);
}

/* Return the physical address corresponding to the page directory */
uint32_t vmm_get_pd_phys(void) { return kernel_pd_phys; }

/* Activate the current page directory in the hardware */
void vmm_activate(void) {
  if (!kernel_pd)
    return;
  arch_set_cr3(kernel_pd_phys);
  arch_enable_paging();
}

/* Map the physical address paddr (must be page-aligned) to the virtual address vaddr */
int vmm_map(void *vaddr, void *paddr, uint32_t flags) {
  if (!kernel_pd)
    return -1;
  uint32_t va = (uint32_t)vaddr;
  uint32_t pa = (uint32_t)paddr;

  if ((va & (VMM_PAGE_SIZE - 1)) || (pa & (VMM_PAGE_SIZE - 1))) {
    return -1; /* Requires page alignment */
  }

  int created = 0;
  vmm_pte_t *pt = get_or_create_pte_table(va, &created);
  if (!pt)
    return -1;

  uint32_t pte_idx = PTE_INDEX(va);
  pt[pte_idx] = make_entry(pa, flags | VMM_P_PRESENT);

  return 0;
}

/* Allocate a physical page for vaddr and map it (same flags as above) */
int vmm_map_page(void *vaddr, uint32_t flags) {
  void *phys = kalloc();
  if (!phys)
    return -1;
  page_zero(phys);
  if (vmm_map(vaddr, phys, flags) != 0) {
    kfree(phys);
    return -1;
  }
  return 0;
}

/* Unmap: if free_phys is not 0, free the physical page back to kfree
 * (only if PTE exists and is present)
 */
int vmm_unmap(void *vaddr, int free_phys) {
  if (!kernel_pd)
    return -1;
  uint32_t va = (uint32_t)vaddr;
  if (va & (VMM_PAGE_SIZE - 1))
    return -1;

  vmm_pte_t *pt = get_pte_table(va);
  if (!pt)
    return -1; /* Unmapped */

  uint32_t pte_idx = PTE_INDEX(va);
  vmm_pte_t pte = pt[pte_idx];
  if ((pte & VMM_P_PRESENT) == 0)
    return -1;

  uint32_t phys_page = pte & 0xFFFFF000u;
  pt[pte_idx] = 0;

  if (free_phys) {
    kfree((void *)(uintptr_t)phys_page);
  }

  /* If the page table is completely empty, you can choose to release the page table
     page and clear the PDE
     (this implementation does not automatically release page table pages)
   */

  return 0;
}

/* Translate virtual address to physical address; return a pointer to the physical address (or NULL)
 */
void *vmm_translate(void *vaddr) {
  if (!kernel_pd)
    return NULL;
  uint32_t va = (uint32_t)vaddr;
  vmm_pte_t *pt = get_pte_table(va);
  if (!pt)
    return NULL;
  vmm_pte_t pte = pt[PTE_INDEX(va)];
  if ((pte & VMM_P_PRESENT) == 0)
    return NULL;
  uint32_t phys = (pte & 0xFFFFF000u) | PAGE_OFFSET(va);
  return (void *)(uintptr_t)phys;
}

void vmm_handle_page_fault(uint32_t fault_addr, uint32_t errcode) {
  printk("\n!!! page fault @ 0x%x, errcode=0x%x\n", fault_addr, errcode);
}

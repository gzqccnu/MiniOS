#include "vmm.h"
#include "../include/log.h"
#include "../string/string.h"


#define ENTRIES_PER_TABLE 1024
#define PDE_INDEX(addr) (((uint32_t)(addr) >> 22) & 0x3FF)
#define PTE_INDEX(addr) (((uint32_t)(addr) >> 12) & 0x3FF)
#define PAGE_OFFSET(addr) ((uint32_t)(addr)&0xFFF)

static vmm_pde_t *kernel_pd = NULL; /* 内核页目录的虚拟地址（可被内核访问） */
static uint32_t kernel_pd_phys = 0; /* 内核页目录对应的物理地址 */

static void page_zero(void *p) {
  uint8_t *b = (uint8_t *)p;
  for (size_t i = 0; i < VMM_PAGE_SIZE; ++i)
    b[i] = 0;
}

extern void arch_set_cr3(uint32_t pd_phys); /* 将 CR3 设为 pd_phys */
extern void arch_enable_paging(void);       /* 设置 CR0 PG 位以启用分页 */

void arch_set_cr3(uint32_t pd_phys) { (void)pd_phys; }
void arch_enable_paging(void) {
}

/* 返回一个新分配并已清零的页（作为页表或页目录页）*/
static void *alloc_page_table_page(void) {
  void *p = kalloc();
  if (!p)
    return NULL;
  page_zero(p);
  return p;
}

/* 将物理地址封装成 PDE/PTE 值（低 12 位为标志） */
static inline vmm_pde_t make_entry(uint32_t paddr, uint32_t flags) {
  return (vmm_pde_t)((paddr & 0xFFFFF000u) | (flags & 0xFFFu));
}

/* 从一个页表（虚拟地址 pointer）获取其物理地址：
*/
static uint32_t virt_to_phys(void *v) {
  return (uint32_t)v;
}

/* 获取或创建页表：若不存在则分配一页并放入页目录 */
static vmm_pte_t *get_or_create_pte_table(uint32_t vaddr, int *created) {
  uint32_t pde_idx = PDE_INDEX(vaddr);
  vmm_pde_t pde = kernel_pd[pde_idx];
  if ((pde & VMM_P_PRESENT) == 0) {
    /* 分配新的页表页 */
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

/* 获取已存在的页表（不创建），若不存在返回 NULL */
static vmm_pte_t *get_pte_table(uint32_t vaddr) {
  uint32_t pde_idx = PDE_INDEX(vaddr);
  vmm_pde_t pde = kernel_pd[pde_idx];
  if ((pde & VMM_P_PRESENT) == 0)
    return NULL;
  uint32_t pt_phys = pde & 0xFFFFF000u;
  return (vmm_pte_t *)(uintptr_t)pt_phys;
}

/* 初始化 VMM：分配并清零内核页目录 */
void vmm_init(void) {
  INFO("vmm: initialize");
  if (kernel_pd)
    return; /* 已初始化 */

  /* 分配页目录页 */
  void *pd_page = alloc_page_table_page();
  if (!pd_page) {
    ERROR("vmm: failed to allocate page directory");
    return;
  }
  kernel_pd = (vmm_pde_t *)pd_page;
  kernel_pd_phys = virt_to_phys(pd_page);

  printk("vmm: page directory created at virt=%p phys=0x%x\n", kernel_pd, kernel_pd_phys);
}

/* 返回当前页目录虚拟地址 */
vmm_pde_t *vmm_get_page_directory(void) { return kernel_pd; }
void vmm_set_page_directory(vmm_pde_t *pd) {
  kernel_pd = pd;
  kernel_pd_phys = virt_to_phys(pd);
}

/* 返回页目录对应的物理地址 */
uint32_t vmm_get_pd_phys(void) { return kernel_pd_phys; }

/* 激活当前页目录到硬件 */
void vmm_activate(void) {
  if (!kernel_pd)
    return;
  arch_set_cr3(kernel_pd_phys);
  arch_enable_paging();
}

/* 将物理地址 paddr（必须页对齐）映射到虚拟地址 vaddr */
int vmm_map(void *vaddr, void *paddr, uint32_t flags) {
  if (!kernel_pd)
    return -1;
  uint32_t va = (uint32_t)vaddr;
  uint32_t pa = (uint32_t)paddr;

  if ((va & (VMM_PAGE_SIZE - 1)) || (pa & (VMM_PAGE_SIZE - 1))) {
    return -1; /* 需要页对齐 */
  }

  int created = 0;
  vmm_pte_t *pt = get_or_create_pte_table(va, &created);
  if (!pt)
    return -1;

  uint32_t pte_idx = PTE_INDEX(va);
  pt[pte_idx] = make_entry(pa, flags | VMM_P_PRESENT);

  return 0;
}

/* 为 vaddr 分配物理页并映射（flags 同上） */
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

/* 解除映射：若 free_phys 非 0 则把物理页释放回 kfree（仅当 PTE 存在且 present） */
int vmm_unmap(void *vaddr, int free_phys) {
  if (!kernel_pd)
    return -1;
  uint32_t va = (uint32_t)vaddr;
  if (va & (VMM_PAGE_SIZE - 1))
    return -1;

  vmm_pte_t *pt = get_pte_table(va);
  if (!pt)
    return -1; /* 未映射 */

  uint32_t pte_idx = PTE_INDEX(va);
  vmm_pte_t pte = pt[pte_idx];
  if ((pte & VMM_P_PRESENT) == 0)
    return -1;

  uint32_t phys_page = pte & 0xFFFFF000u;
  pt[pte_idx] = 0;

  if (free_phys) {
    kfree((void *)(uintptr_t)phys_page);
  }

  /* 若页表已完全空，可以选择释放该页表页并清除 PDE（本实现不自动释放页表页） */

  return 0;
}

/* 翻译虚拟地址到物理地址；返回指向物理地址的指针（或 NULL） */
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

#ifndef VMM_H
#define VMM_H

#include "kmem.h" /* 使用 kalloc/kfree */
#include <stddef.h>
#include <stdint.h>

/* 页尺寸来自 kmem.h 的 PAGE_SIZE（4KB） */
#define VMM_PAGE_SIZE PAGE_SIZE

#define VMM_P_PRESENT 0x1u
#define VMM_P_RW 0x2u   /* 1 = writable */
#define VMM_P_USER 0x4u /* 1 = user-accessible */
#define VMM_P_WRITETHRU 0x8u
#define VMM_P_CACHEDIS 0x10u
#define VMM_P_ACCESSED 0x20u
#define VMM_P_DIRTY 0x40u
#define VMM_P_PS 0x80u /* page size (for PDE large pages) - not used here */

/* 基本类型 */
typedef uint32_t vmm_pde_t;
typedef uint32_t vmm_pte_t;

#define EXPECT(cond, msg)                                                                          \
  if (!(cond)) {                                                                                   \
    printk("TEST FAILED: %s\n", msg);                                                              \
  } else {                                                                                         \
    printk("OK: %s\n", msg);                                                                       \
  }

/* 初始化虚拟内存子系统：创建内核页目录 */
void vmm_init(void);

/* 把物理页 paddr 映射到虚拟地址 vaddr，flags 包含 VMM_P_PRESENT|VMM_P_RW|VMM_P_USER 等 */
int vmm_map(void *vaddr, void *paddr, uint32_t flags);

/* 为 vaddr 分配一个物理页并映射（等价于 map + 为物理页调用 kalloc） */
int vmm_map_page(void *vaddr, uint32_t flags);

/* 解除 vaddr 的映射。如果 free_phys != 0，则释放对应物理页（kfree） */
int vmm_unmap(void *vaddr, int free_phys);

/* 翻译虚拟地址到物理地址（返回物理地址，失败返回 NULL） */
void *vmm_translate(void *vaddr);

/* 激活当前页目录到硬件（调用 arch_set_cr3） */
void vmm_activate(void);

/* 获取当前页目录物理地址（如果有）或 0 */
uint32_t vmm_get_pd_phys(void);

/* 获取/设置内核页目录基址（指针形式） */
vmm_pde_t *vmm_get_page_directory(void);
void vmm_set_page_directory(vmm_pde_t *pd);

/* 简单的页面错误处理钩子（你可以在异常处理程序中调用它）*/
void vmm_handle_page_fault(uint32_t fault_addr, uint32_t errcode);

#endif /* VMM_H */

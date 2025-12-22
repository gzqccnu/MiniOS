/* ============================================
 * kmem.h - Paged Memory Management Header File
 * ============================================ */
#ifndef KMEM_H
#define KMEM_H

#include <stddef.h>
#include <stdint.h>

/* Configuration Parameters */
#define PAGE_SIZE 4096 /* page size：4KB */

/* Page status flag */
#define PAGE_FREE 0
#define PAGE_USED 1

/* Page descriptor structure */
typedef struct Page {
  uint8_t flags;     /* Page status flag */
  struct Page *next; /* Pointer to the next free page (for the free list) */
} Page;

/* Memory Manager Structure */
typedef struct {
  Page *page_array;     /* Array of page descriptors */
  Page *free_list;      /* Head of the free page list */
  void *memory_start;   /* Starting address of memory */
  uint32_t total_pages; /* Total number of pages */
  uint32_t free_pages;  /* Number of free pages */
} MemoryManager;

/* Function Declarations */

/**
 * Initialize the memory manager
 * @param heap_start Start address of the heap (from linker script)
 * @param heap_end End address of the heap (from linker script)
 */
void kinit(void *heap_start, void *heap_end);

/**
 * Allocate one page of memory (4KB)
 * @return Returns the address of the allocated page, NULL if allocation fails
 */
void *kalloc(void);

/**
 * Free one page of memory
 * @param addr Address of the page to be freed
 */
void kfree(void *addr);

/**
 * Get the total number of memory pages
 * @return Total number of pages
 */
uint32_t get_total_pages(void);

/**
 * Get the number of free pages
 * @return Number of free pages
 */
uint32_t get_free_pages(void);

/**
 * Get the number of used pages
 * @return Number of used pages
 */
uint32_t get_used_pages(void);

/**
 * Get the total memory size (bytes)
 * @return Total memory in bytes
 */
size_t get_total_memory(void);

/**
 * Get the free memory size (bytes)
 * @return Free memory in bytes
 */
size_t get_free_memory(void);

/**
 * Print memory statistics (for debugging)
 */
void print_memory_stats(void);

#endif /* KMEM_H */

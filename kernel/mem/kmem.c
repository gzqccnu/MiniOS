#include "kmem.h"
#include "../include/log.h"
#include "../uart/uart.h"
#include <stddef.h>
#include <stdint.h>

/* Global memory manager instance */
static MemoryManager mm;

/**
 * initialize memory manager
 */
void kinit(void *heap_start, void *heap_end) {
  /* compute the size of heap */
  INFO("Initializing Memory Manager...");
  size_t heap_size = (uint8_t *)heap_end - (uint8_t *)heap_start;

  /* Initialize memory manager structure */
  mm.memory_start = heap_start;
  mm.total_pages = heap_size / PAGE_SIZE;
  mm.free_pages = mm.total_pages;

  /* Cannot initialize if the memory is too small */
  if (mm.total_pages == 0) {
    mm.page_array = NULL;
    mm.free_list = NULL;
    return;
  }

  /* The page descriptor array is placed at the beginning of the heap */
  mm.page_array = (Page *)heap_start;
  size_t page_array_size = sizeof(Page) * mm.total_pages;

  /* Calculate the number of pages occupied by the page descriptor */
  uint32_t reserved_pages = (page_array_size + PAGE_SIZE - 1) / PAGE_SIZE;

  /* Initialize all page descriptors */
  for (uint32_t i = 0; i < mm.total_pages; i++) {
    mm.page_array[i].flags = PAGE_FREE;
    mm.page_array[i].next = NULL;
  }

  /* Mark pages occupied by page descriptors as used */
  for (uint32_t i = 0; i < reserved_pages; i++) {
    mm.page_array[i].flags = PAGE_USED;
    mm.free_pages--;
  }

  /**
   * Build the free page list
   * (insert in reverse order so that lower addresses are allocated first)
   */
  mm.free_list = NULL;
  for (uint32_t i = mm.total_pages - 1; i >= reserved_pages; i--) {
    mm.page_array[i].next = mm.free_list;
    mm.free_list = &mm.page_array[i];
    if (i == reserved_pages)
      break; /* Prevent unsigned integer underflow */
  }
  INFO("Memory Manager initialized.");
}

/**
 * Allocate a page of memory
 */
void *kalloc(void) {
  /* Check if there is a free page */
  if (mm.free_list == NULL) {
    return NULL;
  }

  /* Remove a page from the head of the free list */
  Page *page = mm.free_list;
  mm.free_list = page->next;

  /* Marked as used */
  page->flags = PAGE_USED;
  page->next = NULL;
  mm.free_pages--;

  /* Calculate the physical address of the page */
  uint32_t page_index = page - mm.page_array;
  void *addr = (uint8_t *)mm.memory_start + (page_index * PAGE_SIZE);

  /* Clear page content (optional, depending on performance requirements) */
  uint8_t *ptr = (uint8_t *)addr;
  for (size_t i = 0; i < PAGE_SIZE; i++) {
    ptr[i] = 0;
  }

  return addr;
}

/**
 * Free a page of memory
 */
void kfree(void *addr) {
  /* Check for NULL pointer */
  if (addr == NULL) {
    return;
  }

  /* Check if the address is within a valid range */
  if (addr < mm.memory_start ||
      addr >= (void *)((uint8_t *)mm.memory_start + mm.total_pages * PAGE_SIZE)) {
    return;
  }

  /* Calculate page index */
  size_t offset = (uint8_t *)addr - (uint8_t *)mm.memory_start;

  /* Check if the address is page-aligned */
  if (offset % PAGE_SIZE != 0) {
    return;
  }

  uint32_t page_index = offset / PAGE_SIZE;
  Page *page = &mm.page_array[page_index];

  /* Check for double free */
  if (page->flags == PAGE_FREE) {
    return;
  }

  /* Mark as idle */
  page->flags = PAGE_FREE;

  /* Add to the head of the free list */
  page->next = mm.free_list;
  mm.free_list = page;
  mm.free_pages++;
}

/**
 * Get total number of pages
 */
uint32_t get_total_pages(void) { return mm.total_pages; }

/**
 * Get numbers of free pages
 */
uint32_t get_free_pages(void) { return mm.free_pages; }

/**
 * Get number of used of pages
 */
uint32_t get_used_pages(void) { return mm.total_pages - mm.free_pages; }

/**
 * Get total memory size
 */
size_t get_total_memory(void) { return (size_t)mm.total_pages * PAGE_SIZE; }

/**
 * Get size of free memory
 */
size_t get_free_memory(void) { return (size_t)mm.free_pages * PAGE_SIZE; }

void print_memory_stats(void) {
  printk("\n========== memory info ==========\n");
  printk("total pages:   %lu page (%lu byte) \n", mm.total_pages, (mm.total_pages * PAGE_SIZE));
  printk("free pages :   %lu page (%lu byte) \n", mm.free_pages, (mm.free_pages * PAGE_SIZE));
  printk("used pages :   %lu page (%lu byte) \n", get_used_pages(), (get_used_pages() * PAGE_SIZE));
  printk("===================================\n\n");
}

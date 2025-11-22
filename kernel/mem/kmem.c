#include "kmem.h"
#include "../include/log.h"
#include "../uart/uart.h"
#include <stddef.h>
#include <stdint.h>

/* 全局内存管理器实例 */
static MemoryManager mm;

/**
 * 初始化内存管理器
 */
void kinit(void *heap_start, void *heap_end) {
  /* 计算堆的总大小 */
  INFO("Initializing Memory Manager...");
  size_t heap_size = (uint8_t *)heap_end - (uint8_t *)heap_start;

  /* 初始化内存管理器结构 */
  mm.memory_start = heap_start;
  mm.total_pages = heap_size / PAGE_SIZE;
  mm.free_pages = mm.total_pages;

  /* 如果内存太小，无法初始化 */
  if (mm.total_pages == 0) {
    mm.page_array = NULL;
    mm.free_list = NULL;
    return;
  }

  /* 页描述符数组放在堆的开头 */
  mm.page_array = (Page *)heap_start;
  size_t page_array_size = sizeof(Page) * mm.total_pages;

  /* 计算页描述符占用的页数 */
  uint32_t reserved_pages = (page_array_size + PAGE_SIZE - 1) / PAGE_SIZE;

  /* 初始化所有页描述符 */
  for (uint32_t i = 0; i < mm.total_pages; i++) {
    mm.page_array[i].flags = PAGE_FREE;
    mm.page_array[i].next = NULL;
  }

  /* 标记页描述符占用的页为已使用 */
  for (uint32_t i = 0; i < reserved_pages; i++) {
    mm.page_array[i].flags = PAGE_USED;
    mm.free_pages--;
  }

  /* 构建空闲页链表（倒序插入，使得低地址先被分配）*/
  mm.free_list = NULL;
  for (uint32_t i = mm.total_pages - 1; i >= reserved_pages; i--) {
    mm.page_array[i].next = mm.free_list;
    mm.free_list = &mm.page_array[i];
    if (i == reserved_pages)
      break; /* 防止无符号整数下溢 */
  }
  INFO("Memory Manager initialized.");
}

/**
 * 分配一页内存
 */
void *kalloc(void) {
  /* 检查是否有空闲页 */
  if (mm.free_list == NULL) {
    return NULL;
  }

  /* 从空闲链表头部取出一页 */
  Page *page = mm.free_list;
  mm.free_list = page->next;

  /* 标记为已使用 */
  page->flags = PAGE_USED;
  page->next = NULL;
  mm.free_pages--;

  /* 计算页的物理地址 */
  uint32_t page_index = page - mm.page_array;
  void *addr = (uint8_t *)mm.memory_start + (page_index * PAGE_SIZE);

  /* 清零页内容（可选，取决于性能需求）*/
  uint8_t *ptr = (uint8_t *)addr;
  for (size_t i = 0; i < PAGE_SIZE; i++) {
    ptr[i] = 0;
  }

  return addr;
}

/**
 * 释放一页内存
 */
void kfree(void *addr) {
  /* 检查NULL指针 */
  if (addr == NULL) {
    return;
  }

  /* 检查地址是否在有效范围内 */
  if (addr < mm.memory_start ||
      addr >= (void *)((uint8_t *)mm.memory_start + mm.total_pages * PAGE_SIZE)) {
    return;
  }

  /* 计算页索引 */
  size_t offset = (uint8_t *)addr - (uint8_t *)mm.memory_start;

  /* 检查地址是否页对齐 */
  if (offset % PAGE_SIZE != 0) {
    return;
  }

  uint32_t page_index = offset / PAGE_SIZE;
  Page *page = &mm.page_array[page_index];

  /* 检查是否重复释放 */
  if (page->flags == PAGE_FREE) {
    return;
  }

  /* 标记为空闲 */
  page->flags = PAGE_FREE;

  /* 添加到空闲链表头部 */
  page->next = mm.free_list;
  mm.free_list = page;
  mm.free_pages++;
}

/**
 * 获取总页数
 */
uint32_t get_total_pages(void) { return mm.total_pages; }

/**
 * 获取空闲页数
 */
uint32_t get_free_pages(void) { return mm.free_pages; }

/**
 * 获取已使用页数
 */
uint32_t get_used_pages(void) { return mm.total_pages - mm.free_pages; }

/**
 * 获取总内存大小
 */
size_t get_total_memory(void) { return (size_t)mm.total_pages * PAGE_SIZE; }

/**
 * 获取空闲内存大小
 */
size_t get_free_memory(void) { return (size_t)mm.free_pages * PAGE_SIZE; }

void print_memory_stats(void) {
  printk("\n========== 内存统计信息 ==========\n");
  printk("总页数:   %lu 页 (%lu byte)\n", mm.total_pages, (mm.total_pages * PAGE_SIZE));
  printk("空闲页:   %lu 页 (%lu byte)\n", mm.free_pages, (mm.free_pages * PAGE_SIZE));
  printk("已用页:   %lu 页 (%lu byte)\n", get_used_pages(), (get_used_pages() * PAGE_SIZE));
  printk("==================================\n\n");
}

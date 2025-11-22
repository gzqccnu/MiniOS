/* ============================================
 * kalloc.h - 页式内存管理头文件
 * ============================================ */
#ifndef KMEM_H
#define KMEM_H

#include <stddef.h>
#include <stdint.h>

/* 配置参数 */
#define PAGE_SIZE 4096 /* 页大小：4KB */

/* 页的状态标志 */
#define PAGE_FREE 0
#define PAGE_USED 1

/* 页描述符结构 */
typedef struct Page {
  uint8_t flags;     /* 页状态标志 */
  struct Page *next; /* 指向下一个空闲页（用于空闲链表）*/
} Page;

/* 内存管理器结构 */
typedef struct {
  Page *page_array;     /* 页描述符数组 */
  Page *free_list;      /* 空闲页链表头 */
  void *memory_start;   /* 内存起始地址 */
  uint32_t total_pages; /* 总页数 */
  uint32_t free_pages;  /* 空闲页数 */
} MemoryManager;

/* 函数声明 */

/**
 * 初始化内存管理器
 * @param heap_start 堆起始地址（来自链接器脚本）
 * @param heap_end 堆结束地址（来自链接器脚本）
 */
void kinit(void *heap_start, void *heap_end);

/**
 * 分配一页内存（4KB）
 * @return 返回分配的页的地址，失败返回NULL
 */
void *kalloc(void);

/**
 * 释放一页内存
 * @param addr 要释放的页地址
 */
void kfree(void *addr);

/**
 * 获取总内存页数
 * @return 总页数
 */
uint32_t get_total_pages(void);

/**
 * 获取空闲页数
 * @return 空闲页数
 */
uint32_t get_free_pages(void);

/**
 * 获取已使用页数
 * @return 已使用页数
 */
uint32_t get_used_pages(void);

/**
 * 获取总内存大小（字节）
 * @return 总内存字节数
 */
size_t get_total_memory(void);

/**
 * 获取空闲内存大小（字节）
 * @return 空闲内存字节数
 */
size_t get_free_memory(void);

/**
 * 打印内存统计信息（调试用）
 */
void print_memory_stats(void);

#endif /* KMEM_H */
/*
 * kheap.c - This file implements different kernel heap
 * functions for memory
 *
 * Author: Fedi Nabli
 * Date: 5 Mar 2025
 * Last Modified: 6 Mar 2025
 */

#include "kheap.h"

#include <uart.h>
#include <heap.h>

#include <kernel/config.h>
#include <synapse/memory/memory.h>

struct heap kernel_heap;
struct heap_table kernel_heap_table;

extern char _end;

/**
 * @brief Initializes the kernel heap with dynamic size based on
 * available RAM
 * 
 * @author Fedi Nabli
 * @date 6 Mar 2025
 * 
 * @param ram_size total amount of available ram from boot_info structure 
 */
void kheap_init(size_t ram_size)
{
  size_t target_heap_size = ram_size / 5;
  const size_t MIN_HEAP_SIZE = 4 * 1024 * 1024; // 4MB minimum
  const size_t MAX_HEAP_SIZE = 256 * 1024 * 1024; // 256MB maximum

  if (target_heap_size < MIN_HEAP_SIZE)
    target_heap_size = MIN_HEAP_SIZE;
  if (target_heap_size > MAX_HEAP_SIZE)
    target_heap_size = MAX_HEAP_SIZE;

  target_heap_size = (target_heap_size / KERNEL_HEAP_BLOCK_SIZE) * KERNEL_HEAP_BLOCK_SIZE;

  size_t total_table_entries = target_heap_size / KERNEL_HEAP_BLOCK_SIZE;
  size_t table_size = total_table_entries * sizeof(HEAP_BLOCK_TABLE_ENTRY);

  uintptr_t kernel_end = (uintptr_t)&_end;

  uintptr_t aligned_end = (kernel_end + 0xFFF) & ~0xFFF;

  void* heap_table_addr = (void*)aligned_end;
  void* heap_start_addr = (void*)(aligned_end + table_size);

  heap_start_addr = (void*)(((uintptr_t)heap_start_addr + 0xFFF) & ~0xFFF);

  // Initialize heap table
  kernel_heap_table.entries = (HEAP_BLOCK_TABLE_ENTRY*)heap_table_addr;
  kernel_heap_table.total = total_table_entries;

  // Create the heap
  void* heap_end_addr = (void*)((size_t)heap_start_addr + target_heap_size);
  int res = heap_create(&kernel_heap, heap_start_addr, heap_end_addr, &kernel_heap_table);

  if (res < 0)
  {
    // TODO: handle error
    uart_send_string("Panic");
  }
}

/**
 * @brief Allocated requested amount in the memory
 * and returns the address
 * 
 * @author Fedi Nabli
 * @date 6 Mar 2025
 * 
 * @param size amount of memory needed in bytes 
 * @return void* start address of the memory allocated
 */
void* kmalloc(size_t size)
{
  return heap_malloc(&kernel_heap, size);
}

/**
 * @brief Frees memory region previously allocated to ptr
 * 
 * @author Fedi Nabli
 * @date 6 Mar 2025
 * 
 * @param ptr the address of the variable
 */
void kfree(void* ptr)
{
  heap_free(&kernel_heap, ptr);
}

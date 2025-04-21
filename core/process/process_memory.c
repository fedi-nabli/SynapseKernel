/*
 * process_memory.c - Process memory management for AArch64-based kernel
 *
 * Author: Fedi Nabli
 * Date: 3 Apr 2025
 * Last Modified: 7 Apr 2025
 */

#include "process.h"

#include <uart.h>

#include <kernel/config.h>

#include <synapse/status.h>

#include <synapse/memory/memory.h>
#include <synapse/memory/heap/kheap.h>

/**
 * @brief Find free allocation entry in process
 * 
 * @param process Process to search
 * @return int Index of free entry, or negative error code
 * 
 * @author Fedi Nabli
 * @date 3 Apr 20225
 */
static int process_find_free_allocation_index(struct process* process)
{
  if (!process)
  {
    return -EINVARG;
  }

  for (size_t i = 0; i < SYNAPSE_MAX_PROCESSES_ALLOCATIONS; i++)
  {
    if (process->allocations[i].ptr == NULL)
    {
      return i;
    }
  }

  // Maximum allocations reached
  return -EPMAX;
}

/**
 * @brief Allocate memory for a process
 * 
 * @param process Process to allocate for
 * @param size Size in bytes to allocatr
 * @return void* Pointer to allocated memory, NULL on failure
 * 
 * @author Fedi Nabli
 * @date 7 Apr 2025
 */
void* process_malloc(struct process* process, size_t size)
{
  if (!process || size == 0)
  {
    if (size == 0)
    {
      uart_send_string("process_malloc: size == 0\n");
    }
    else
    {
      uart_send_string("process_malloc: Invalid process\n");
    }
    return NULL;
  }

  // Find free allocation entry
  int index = process_find_free_allocation_index(process);
  if (index < 0)
  {
    uart_send_string("process_malloc: No free allocation entry\n");
    return NULL;
  }

  // Allocate memory
  void* ptr = kmalloc(size);
  if (!ptr)
  {
    uart_send_string("process_malloc: Kmalloc returned no pointer\n");
    return NULL;
  }

  // Record only the actual requested size in the allocation table
  process->allocations[index].ptr = ptr;
  process->allocations[index].size = size;

  uart_send_string("process_malloc: Finished memory allocation for process\n");

  return ptr;
}

/**
 * @brief Free memory allocated to a process
 * 
 * @param process Process that owns the memory
 * @param ptr Pointer to memory to free
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 7 Apr 2025
 */
int process_free(struct process* process, void* ptr)
{
  if (!process || !ptr)
  {
    return -EINVARG;
  }

  // Find allocation entry
  for (size_t i = 0; i < SYNAPSE_MAX_PROCESSES_ALLOCATIONS; i++)
  {
    if (process->allocations[i].ptr == ptr)
    {
      // Free memory
      kfree(ptr);

      // Clear memory entry
      process->allocations[i].ptr = NULL;
      process->allocations[i].size = 0;

      return EOK;
    }
  }

  // Allocation not found
  return -EINVARG;
}

/**
 * @brief Get total memory allocated to a process
 * 
 * @param process Process to check
 * @return size_t Total bytes allocated
 * 
 * @author Fedi Nabli
 * @date 7 Apr 2025
 */
size_t process_get_memory_usage(struct process* process)
{
  if (!process)
  {
    return 0;
  }

  size_t total = 0;

  // Sum all allocations
  for (int i = 0; i < SYNAPSE_MAX_PROCESSES_ALLOCATIONS; i++)
  {
    if (process->allocations[i].ptr != NULL)
    {
      total += process->allocations[i].size;
    }
  }

  // Add program size
  total += process->size;

  return total;
}

/**
 * @brief Verify if memory address belongs to a process
 * 
 * This function is particularly important for AArch64 without MMU/paging
 * as we need to validate memory access from user space
 * 
 * @param process Process to check
 * @param addr Memory address to verify
 * @param size Size of memory range to check
 * @return true if the memory range belongs to the process
 * @return false if the memory range doesn't belong to the process
 * 
 * @author Fedi Nabli
 * @date 7 Apr 2025
 */
bool process_memory_verify(struct process* process, void* addr, size_t size)
{
  if (!process || !addr || size == 0)
  {
    return false;
  }

  uint64_t start_addr = (uint64_t)addr;
  uint64_t end_addr = start_addr + size - 1;

  // Check if address is within stack
  if (process->stack)
  {
    uint64_t stack_start = (uint64_t)process->stack;
    uint64_t stack_end = stack_start + SYNAPSE_PROCESS_STACK_SIZE - 1;

    if (start_addr >= stack_start && end_addr <= stack_end)
    {
      return true;
    }
  }

  // Check if address is within program code
  if (process->ptr)
  {
    uint64_t code_start = (uint64_t)process->ptr;
    uint64_t code_end = code_start + process->size - 1;

    if (start_addr >= code_start && end_addr <= code_end)
    {
      return true;
    }
  }

  // Check if address is within any allocation
  for (int i = 0; i < SYNAPSE_MAX_PROCESSES_ALLOCATIONS; i++)
  {
    if (process->allocations[i].ptr)
    {
      uint64_t alloc_start = (uint64_t)process->allocations[i].ptr;
      uint64_t alloc_end = alloc_start + process->allocations[i].size - 1;

      if (start_addr >= alloc_start && end_addr <= alloc_end)
      {
        return true;
      }
    }
  }

  return false;
}

/**
 * @brief Flush instruction cache for memory region
 * 
 * This is important for AArch64 when modifying code in memory
 * 
 * @param addr Start address
 * @param size Size of region
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 7 Apr 2025
 */
int process_memory_flush_icache(void* addr, size_t size)
{
  // AArch64 cache maintenance operations
  uint64_t end_addr = (uint64_t)addr + size;

  // Cache line aligned addresses
  uint64_t line_size = 64;
  uint64_t start_aligned = (uint64_t)addr & ~(line_size - 1);
  uint64_t end_aligned = (end_addr + line_size - 1) & ~(line_size - 1);

  // Flush data cache to point of coherency
  for (uint64_t current = start_aligned; current < end_aligned; current += line_size)
  {
    __asm__ volatile("dc civac, %0" :: "r"(current));
  }

  // Instruction synchronization barrier
  __asm__ volatile("isb");

  // Data synchronization barrier
  __asm__ volatile("dsb ish");

  // Invalidate instruction cache to point of unification
  for (uint64_t current = start_aligned; current < end_aligned; current += line_size)
  {
    __asm__ volatile("ic ivau, %0" :: "r"(current));
  }

  // Instruction synchronization barrier
  __asm__ volatile("isb");

  return EOK;
}

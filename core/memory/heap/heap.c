/*
 * heap.c - This file implements different methods related to
 * the kernel heap region including init, malloc and free
 * defined in heap.h
 *
 * Author: Fedi Nabli
 * Date: 4 Mar 2025
 * Last Modified: 5 Mar 2025
 */

#include "heap.h"

#include <kernel/config.h>

#include <synapse/bool.h>
#include <synapse/types.h>
#include <synapse/status.h>
#include <synapse/memory/memory.h>

static int heap_validate_table(void* ptr, void* end, struct heap_table* table)
{
  int res = 0;

  size_t table_size = (size_t)(end - ptr);
  size_t total_blocks = table_size / KERNEL_HEAP_BLOCK_SIZE;
  if (table->total != total_blocks)
  {
    res = -EINVARG;
    goto out;
  }

out:
  return res;
}

static bool heap_validate_alignment(void* ptr)
{
  return ((size_t)ptr % KERNEL_HEAP_BLOCK_SIZE) == 0;
}

static uint32_t heap_align_value_to_upper(uint32_t val)
{
  if ((val % KERNEL_HEAP_BLOCK_SIZE) == 0)
  {
    return val;
  }

  val = (val - (val % KERNEL_HEAP_BLOCK_SIZE));
  val += KERNEL_HEAP_BLOCK_SIZE;
  return val;
}

static int heap_get_entry_type(HEAP_BLOCK_TABLE_ENTRY entry)
{
  return entry & 0x0F;
}

static int heap_get_start_block(struct heap* heap, uint32_t total_blocks)
{
  struct heap_table* table = heap->table;
  uint32_t bc = 0; // current block
  int bs = -1; // start block

  for (size_t i = 0; i < table->total; i++)
  {
    if (heap_get_entry_type(table->entries[i]) != HEAP_BLOCK_TABLE_ENTRY_FREE)
    {
      bc = 0;
      bs = -1;
      continue;
    }

    // If this is the first block
    if (bs == -1)
    {
      bs = i;
    }
    bc++;

    if (bc == total_blocks)
    {
      break;
    }
  }

  if (bs == -1)
  {
    return -ENOMEM;
  }

  return bs;
}

static void* heap_block_to_address(struct heap* heap, int start_block)
{
  return heap->saddr + (start_block * KERNEL_HEAP_BLOCK_SIZE);
}

static void heap_mark_blocks_taken(struct heap* heap, int start_block, uint32_t total_blocks)
{
  int end_block = (start_block + total_blocks) - 1;

  HEAP_BLOCK_TABLE_ENTRY entry = HEAP_BLOCK_TABLE_ENTRY_TAKEN | HEAP_BLOCK_IS_FIRST;
  if (total_blocks > 1)
  {
    entry |= HEAP_BLOCK_HAS_NEXT;
  }

  for (int i = start_block; i <= end_block; i++)
  {
    heap->table->entries[i] = entry;
    entry = HEAP_BLOCK_TABLE_ENTRY_TAKEN;
    if (i != end_block)
    {
      entry |= HEAP_BLOCK_HAS_NEXT;
    }
  }
}

static void* heap_malloc_blocks(struct heap* heap, uint32_t total_blocks)
{
  void* address = NULL;

  int start_block = heap_get_start_block(heap, total_blocks);
  if (start_block < 0)
  {
    goto out;
  }

  address = heap_block_to_address(heap, start_block);

  // Mark the blocks as taken
  heap_mark_blocks_taken(heap, start_block, total_blocks);

out:
  return address;
}

static int heap_address_to_block(struct heap* heap, void* address)
{
  return ((size_t)(address - heap->saddr)) / KERNEL_HEAP_BLOCK_SIZE;
}

static void heap_mark_blocks_free(struct heap* heap, int start_block)
{
  struct heap_table* table = heap->table;
  for (int i = start_block; i < (int)table->total; i++)
  {
    HEAP_BLOCK_TABLE_ENTRY entry = table->entries[i];
    table->entries[i] = HEAP_BLOCK_TABLE_ENTRY_FREE;
    if (!(entry & HEAP_BLOCK_HAS_NEXT))
    {
      break;
    }
  }
}

/**
 * @brief Initialize the heap structure and verifies the alignment
 * 
 * @author Fedi Nabli
 * @date 4 Mar 2025
 * 
 * @param heap a pointer to the create structure
 * @param ptr the start address of the heap
 * @param end the end address of the heap
 * @param table the pointer to the heap table containing entries and the total size
 * @return 0 if operation successfull and negative if there was an error
 */
int heap_create(struct heap* heap, void* ptr, void* end, struct heap_table* table)
{
  int res = 0;

  if (!heap_validate_alignment(ptr) || !heap_validate_alignment(end))
  {
    res = -EINVARG;
    goto out;
  }

  memset(heap, 0, sizeof(struct heap));
  heap->saddr = ptr;
  heap->table = table;

  res = heap_validate_table(ptr, end, table);
  if (res < 0)
  {
    goto out;
  }

  size_t table_size = sizeof(HEAP_BLOCK_TABLE_ENTRY) * table->total;
  memset(table->entries, HEAP_BLOCK_TABLE_ENTRY_FREE, table_size);

out:
  return res;
}

/**
 * @brief Allocates memory on the heap from the requested size aligned to 4KB
 * 
 * @author Fedi Nabli
 * @date 5 Mar 2025
 * 
 * @param heap pointer to the kernel heap
 * @param size requested size
 * @return void* address of the starting block (memory address)
 */
void* heap_malloc(struct heap* heap, size_t size)
{
  size_t aligned_size = heap_align_value_to_upper(size);
  uint32_t total_blocks = aligned_size / KERNEL_HEAP_BLOCK_SIZE;
  return heap_malloc_blocks(heap, total_blocks);
}

/**
 * @brief Frees memory previously allocated by heap_malloc
 * 
 * @author Fedi Nabli
 * @date 5 Mar 2025
 * 
 * @param heap pointer to the kernel heap
 * @param ptr pointer variable returned by heap_malloc
 */
void heap_free(struct heap* heap, void* ptr)
{
  heap_mark_blocks_free(heap, heap_address_to_block(heap, ptr));
}

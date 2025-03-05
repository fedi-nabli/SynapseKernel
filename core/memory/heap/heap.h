/*
 * heap.h - This file defines different methods related to
 * the kernel heap region including init, malloc and free.
 *
 * Author: Fedi Nabli
 * Date: 3 Mar 2025
 * Last Modified: 5 Mar 2025
 */

#ifndef __SYNAPSE_MEMORY_HEAP_H_
#define __SYNAPSE_MEMORY_HEAP_H_

#include <synapse/types.h>

#define HEAP_BLOCK_TABLE_ENTRY_FREE 0x00
#define HEAP_BLOCK_TABLE_ENTRY_TAKEN 0x01

#define HEAP_BLOCK_HAS_NEXT 0b10000000
#define HEAP_BLOCK_IS_FIRST 0b01000000

typedef unsigned long HEAP_BLOCK_TABLE_ENTRY;

struct heap_table
{
  HEAP_BLOCK_TABLE_ENTRY* entries;
  size_t total;
};

struct heap
{
  // Pointer to the heap table entries
  struct heap_table* table;
  // start address of the heap
  void* saddr;
};

/**
 * @brief Initialize the heap structure and verifies the alignment
 * 
 * @param heap a pointer to the create structure
 * @param ptr the start address of the heap
 * @param end the end address of the heap
 * @param table the pointer to the heap table containing entries and the total size
 * @return 0 if operation successfull and negative if there was an error
 */
int heap_create(struct heap* heap, void* ptr, void* end, struct heap_table* table);

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
void* heap_malloc(struct heap* heap, size_t size);

/**
 * @brief Frees memory previously allocated by heap_malloc
 * 
 * @author Fedi Nabli
 * @date 5 Mar 2025
 * 
 * @param heap pointer to the kernel heap
 * @param ptr pointer variable returned by heap_malloc
 */
void heap_free(struct heap* heap, void* ptr);

#endif
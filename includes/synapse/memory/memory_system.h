#ifndef __SYNAPSE_MEMORY_SYSTEM_H_
#define __SYNAPSE_MEMORY_SYSTEM_H_

#include <synapse/bool.h>
#include <synapse/types.h>

// Memory region types for mapping
typedef enum
{
  MEM_REGION_TYPE_RAM = 0,
  MEM_REGION_TYPE_DEVICE,
  MEM_REGION_TYPE_MMIO,
  MEM_REGION_TYPE_KERNEL
} mem_region_type_t;

// Memory region structure
typedef struct
{
  uintptr_t phys_start; // Physical start address
  uintptr_t phys_end; // Physical end address
  uintptr_t virt_start; // Virtual start address (if mapped)
  size_t size; // Region size
  mem_region_type_t type; // Region type
  char name[32]; // Region name
} mem_system_region_t;

/**
 * @brief Initialize the complete memory system
 * 
 * This function initializes the entire memory system, including the kernel heap,
 * memory management unit (MMU), paging system, and AI memory subsystem. It also
 * registers memory regions for tracking and creates necessary identity mappings.
 * 
 * @param ram_size The total size of the RAM in bytes
 * @param kernel_start The start address of the kernel in physical memory
 * @param kernel_end The end address of the kernel in physical memory
 * @return int Returns EOK on success, or a negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 21 Mar 2025
 */
int memory_system_init(size_t ram_size, uintptr_t kernel_start, uintptr_t kernel_end);

int memory_test_kernel_heap(void);
int memory_test_paging(void);
int memory_test_ai_memory(void);
int memory_test_regions(void);
void memory_print_regions(void);
int memory_run_tests(void);

#endif
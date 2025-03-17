/*
 * paging.h - Paging Manager
 *
 * This file defines an enhanced paging system that provides
 * convenient abstractions over the MMU operations
 * 
 * Author: Fedi Nabli
 * Date: 16 Mar 2025
 * Last Modified: 16 Mar 2025
 */

#ifndef __SYNAPSE_MEMORY_PAGING_H_
#define __SYNAPSE_MEMORY_PAGING_H_

#include <synapse/bool.h>
#include <synapse/types.h>

/* Page sizes */
#define PAGING_SIZE_4KB   0x1000      // 4KB (Level 3)
#define PAGING_SIZE_2MB   0x200000    // 2MB (Level 2)
#define PAGING_SIZE_1GB   0x40000000  // 1GB (Level 1)

/* Memory region attribute flags */
#define PAGE_READ     (1UL << 0) // Readable
#define PAGE_WRITE    (1UL << 1) // Writeable
#define PAGE_EXEC     (1UL << 2) // Executable
#define PAGE_USER     (1UL << 3) // Accessible from user mode
#define PAGE_NOCACHE  (1UL << 4) // Non-Cacheable
#define PAGE_DEVICE   (1UL << 5) // Device memory
#define PAGE_SHARED   (1UL << 6) // Shareable memory

/* Predefined attribute combinations */
#define PAGE_KERNEL_RO      (PAGE_READ)
#define PAGE_KERNEL_RW      (PAGE_READ | PAGE_WRITE)
#define PAGE_KERNEL_CODE    (PAGE_READ | PAGE_EXEC)
#define PAGE_USER_RO        (PAGE_READ | PAGE_USER)
#define PAGE_USER_RW        (PAGE_READ | PAGE_WRITE | PAGE_USER)
#define PAGE_USER_CODE      (PAGE_READ | PAGE_EXEC | PAGE_USER)
#define PAGE_DEVICE_MEM     (PAGE_READ | PAGE_WRITE | PAGE_NOCACHE | PAGE_DEVICE)

/* Memory regions */
typedef enum
{
  REGION_KERNEL_CODE,     // Kernel code (RX)
  REGION_KERNEL_RODATA,   // Kernel read-only data (RO)
  REGION_KERNEL_DATA,     // Kernel read-write data (RW)
  REGION_KERNEL_HEAP,     // Kernel heap (RW)
  REGION_KERNEL_STACK,    // Kernel stack (RW)
  REGION_USER_CODE,       // User code (RX, user)
  REGION_USER_DATA,       // User data (RW, user)
  REGION_USER_HEAP,       // User heap (RW, user)
  REGION_USER_STACK,      // User stack (RW, user)
  REGION_DEVICE,          // Device memory (RW, no cache)
  REGION_AI_TENSOR,       // AI tensor memory (RW, optimized)
  REGION_MAX
} memory_region_t;

/**
 * @brief Initialize the paging system
 * 
 * @author Fedi Nabli
 * @date 16 Mar 2025
 */
int paging_init();

/**
 * @brief Map a physical memory range to a virtual address
 * 
 * @param vaddr Virtual address to map to
 * @param paddr Physical address to map from
 * @param size size of the memory
 * @param flags flags for the memory mapping
 * @return int 0 if all ok, non-zero if failure
 * 
 * @author Fedi Nabli
 * @date 16 Mar 2025
 */
int paging_map(uint64_t vaddr, uint64_t paddr, size_t size, uint32_t flags);

/**
 * @brief Unmap a virtual address range
 * 
 * @param vaddr Virtual address to unmap
 * @param size Size of the memory range to unmap
 * @return int 0 if all ok, non-zero if failure
 * 
 * @author Fedi Nabli
 * @date 17 Mar 2025
 */
int paging_unmap(uint64_t vaddr, size_t size);

/**
 * @brief Allocate and map virtual memory from a specific region
 * 
 * @param region Memory region to allocate from
 * @param size Size of the memory to allocate
 * @param flags Flags for the memory allocation
 * @return uint64_t Virtual address of the allocated memory, or 0 on failure
 * 
 * @author Fedi Nabli
 * @date 17 Mar 2025
 */
uint64_t paging_alloc_from_region(memory_region_t region, size_t size, uint32_t flags);

/**
 * @brief Allocates a tensor in the paging memory system.
 * 
 * This function allocates a block of memory of the specified size and applies the given flags to it.
 * 
 * @param size The size of the memory block to allocate.
 * @param flags The flags to apply to the allocated memory block.
 * @return uint64_t The address of the allocated memory block.
 */
uint64_t paging_alloc_tensor(size_t size, uint32_t flags);

/**
 * @brief Free memory allocated with paging_alloc functions
 * 
 * @param vaddr Virtual address of the memory to free
 * @return int 0 if successful, non-zero if failure
 * 
 * @author Fedi Nabli
 * @date 17 Mar 2025
 */
int paging_free(uint64_t vaddr);

/**
 * @brief Get physical address for a virtual address
 * 
 * @param vaddr Virtual address to translate
 * @return uint64_t Physical address corresponding to the virtual address, or 0 if not mapped
 * 
 * @author Fedi Nabli
 * @date 17 Mar 2025
 */
uint64_t paging_get_phys_addr(uint64_t vaddr);

/**
 * @brief Set memory protection for a region
 * 
 * @param vaddr Virtual address of the region
 * @param size Size of the region
 * @param flags New protection flags
 * @return int 0 if successful, non-zero if failure
 * 
 * @author Fedi Nabli
 * @date 17 Mar 2025
 */
int paging_set_protection(uint64_t vaddr, size_t size, uint32_t flags);

/**
 * @brief Dump paging system information
 * 
 * @param vaddr Virtual address to dump information for, or 0 for summary
 * 
 * @author Fedi Nabli
 * @date 17 Mar 2025
 */
void paging_dump_info(uint64_t vaddr);

#endif
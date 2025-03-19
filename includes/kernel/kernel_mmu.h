/*
 * kernel_mmu.h - This file defines different kernel mmu
 * functions for memory management
 *
 * Author: Fedi Nabli
 * Date: 19 Mar 2025
 * Last Modified: 20 Mar 2025
 */

#ifndef __KERNEL_MMU_H_
#define __KERNEL_MMU_H_

#include <synapse/types.h>

#define KERNEL_VIRT_BASE 0xFFFF000000000000ULL // High canonical address for kernel sapce
#define PAGE_SIZE 4096 // 4KB page size
#define PAGE_SHIFT 12 // log2(PAGE_SIZE)

// Number of page table levels for 4KB pages with 48-bit VA
#define PAGE_LEVELS 4

// Constants for page table indices
#define PGD_SHIFT 39
#define PUD_SHIFT 30
#define PMD_SHIFT 21
#define PTE_SHIFT 12

#define PGD_MASK 0x1FF
#define PUD_MASK 0x1FF
#define PMD_MASK 0x1FF
#define PTE_MASK 0x1FF

// Memory regions
typedef enum
{
  MEM_REGION_RAM = 0,
  MEM_REGION_DEVICE,
  MEM_REGION_MMIO,
  MEM_REGION_KERNEL
} mem_region_t;

// Memory mapping flags
typedef enum
{
  MAP_READ = (1 << 0), // Readable
  MAP_WRITE = (1 << 1), // Writeable
  MAP_EXEC = (1 << 2), // Executable
  MAP_DEVICE = (1 << 3), // Device memory
  MAP_CACHE_WB = (1 << 4), // Write-back caching
  MAP_CACHE_WT = (1 << 5), // Write-through caching
  MAP_CACHE_NC = (1 << 6), // Non-cacheable
  MAP_USER = (1 << 7), // User accessible
  MAP_SHARED = (1 << 8) // Shareable
} map_flags_t;

/**
 * @brief Initialize the kernel MMU
 * 
 * @param ram_size Size of available RAM
 * @return int 0 if successful, non-zero on failure
 * 
 * @author Fedi Nabli
 * @date 19 Mar 2025
 */
int kernel_mmu_init(size_t ram_size);

/**
 * @brief Enable the MMU
 * 
 * @return int 0 if successful, non-zero on failure
 * 
 * @author Fedi Nabli
 * @date 19 Mar 2025
 */
int kernel_mmu_enable();

/**
 * @brief Create a mapping in the page tables
 * 
 * @param phys_addr The physical address to map
 * @param virt_addr The virtual address to map to
 * @param size The size of the mapping
 * @param flags The mapping flags
 * @return int 0 if successful, non-zero on failure
 * 
 * @author Fedi Nabli
 * @date 19 Mar 2025
 */
int kernel_mmu_map(uintptr_t phys_addr, uintptr_t virt_addr, size_t size, uint32_t flags);

/**
 * @brief Create identity mapping
 * 
 * @param phys_addr The physical address to map
 * @param size The size of the mapping
 * @param flags The mapping flags
 * @return int 0 if successful, non-zero on failure
 * 
 * @author Fedi Nabli
 * @date 19 Mar 2025
 */
int kernel_mmu_identity_map(uintptr_t phys_addr, size_t size, uint32_t flags);

/**
 * @brief Unmap a virtual memory region
 * 
 * @param virt_addr The virtual address to unmap
 * @param size The size of the range to unmap
 * @return int 0 if successful, non-zero on failure
 * 
 * @author Fedi Nabli
 * @date 19 Mar 2025
 */
int kernel_mmu_unmap(uintptr_t virt_addr, size_t size);

/**
 * @brief Translate a virtual address to a physical address
 * 
 * @param virt_addr The virtual address to translate
 * @param phys_addr Pointer to store the resulting physical address
 * @return int 0 if successful, non-zero on failure
 * 
 * @author Fedi Nabli
 * @date 19 Mar 2025
 */
int kernel_mmu_translate(uintptr_t virt_addr, uintptr_t* phys_addr);

/**
 * @brief Get the mapping flags for a virtual address
 * 
 * @param virt_addr The virtual address to query
 * @param flags Pointer to store the resulting flags
 * @return int 0 if successful, non-zero on failure
 * 
 * @author Fedi Nabli
 * @date 20 Mar 2025
 */
int kernel_mmu_get_flags(uintptr_t virt_addr, uint32_t* flags);

/**
 * @brief Set the mapping flags for a virtual address
 * 
 * @param virt_addr The virtual address to set flags for
 * @param flags The new mapping flags
 * @return int 0 if successful, non-zero on failure
 * 
 * @author Fedi Nabli
 * @date 20 Mar 2025
 */
int kernel_mmu_set_flags(uintptr_t virt_addr, uint32_t flags);

/**
 * @brief Print the MMU configuration for debugging
 * 
 * @author Fedi Nabli
 * @date 20 Mar 2025
 */
void kernel_mmu_print_config();

#endif
/*
 * mmu.h - This files defines kernel MMU functionalities
 * 
 * The functions are meant to control the MMU and
 * be architecture-independant
 * 
 * Author: Fedi Nabli
 * Date: 12 Mar 2025
 * Last Modified: 14 Mar 2025
*/

#ifndef __KERNEL_MMU_H_
#define __KERNEL_MMU_H_

#include <synapse/types.h>

/* Generic MMU Constants */
#define PAGE_SHIFT    12 // 4KB pages
#define PAGE_SIZE     (1UL << PAGE_SHIFT)
#define PAGE_MASK     (~(PAGE_SIZE - 1))
#define TABLE_SHIFT   9 // 512 entries per table
#define TABLE_SIZE    (1UL << TABLE_SHIFT)

/* Memory indices for use with MAIR_EL1 - used by the reset of the kernel */
#define MEMORY_ATTR_DEVICE_nGnRnE   0 // Device memory non-Gathering, non-Recording, no Early write acknowledgement
#define MEMORY_ATTR_DEVICE_nGnRE    1 // Device memory non-Gathering, non-Recording, Early write acknowledgement
#define MEMORY_ATTR_DEVICE_GRE      2 // Device memory Gathering, Recording, Early write acknowledgement
#define MEMORY_ATTR_NORMAL_NC       3 // Normal memory, Outer and Inner Non-Cacheable
#define MEMORY_ATTR_NORMAL_WT       4 // Normal memory, Outer and Inner Write-Through, Read-Allocate
#define MEMORY_ATTR_NORMAL_WB       5 // Normal memory, Outer and Inner Write-Back, Read_Allocate, Write-Allocate

/* Memory access permissions - architecture-independant definitions */
#define MMU_PERM_READ       (1UL << 0)
#define MMU_PERM_WRITE      (1UL << 1)
#define MMU_PERM_EXECUTE    (1UL << 2)
#define MMU_PERM_USER       (1UL << 3)
#define MMU_PERM_KERNEL     (1UL << 4)
#define MMU_PERM_CACHED     (1UL << 5)
#define MMU_PERM_UNCACHED   (1UL << 6)
#define MMU_PERM_DEVICE     (1UL << 7)

/**
 * @brief Initializes the MMU
 * 
 * The function initializes the MMU by:
 * 1. Configuring MMU-related system registers
 * 2. Seeting up initial memory mappings
 * 
 * @warning This function does NOT enable the MMU. Call mmu_enable() seperatly.
 * 
 * @author Fedi Nabli
 * @date 13 Mar 2025
 */
void mmu_init();

/**
 * @brief Enable the MMU
 * 
 * This function enables the MMU by setting the M bit in SCTLR_EL1
 * 
 * @return 0 on success, non-zero on failure 
 * 
 * @author Fedi Nabli
 * @date 14 Mar 2025
 */
int mmu_enable();

/**
 * @brief Disables the MMU
 * 
 * This function disables the MMU by clearig the M bit in SCTLR_EL1
 * 
 * @return 0 on success, non-zero on failure
 * 
 * @author Fedi Nabli
 * @date 14 Mar 2025
 */
int mmu_disable();

/**
 * @brief Setup inital memory mappings
 * 
 * This function sets up the initial memory mappings for the kernel.
 * It creates identity mappings for RAM and device memory regions.
 * 
 * @author Fedi Nabli
 * @date 13 Mar 2025
 */
void mmu_setup_initial_mappings();

/**
 * @brief Maps a physical address to a virtual address
 * 
 * This is a basic implementation that will be expanded when implementing paging.
 * Currently just ensures the physical address is covered by our identity mapping.
 * 
 * @param vaddr Virtual address to map to
 * @param paddr Physical address to map from
 * @param size Size of the region to map to
 * @param attrs Memory attributes for the mapping
 * @return 0 on success, non-zero on failure
 * 
 * @author Fedi Nabli
 * @date 14 Mar 2025
 */
int mmu_map(uint64_t vaddr, uint64_t paddr, uint64_t size, uint64_t attrs);

/**
 * @brief Unmaps a virtual address
 * 
 * @param vaddr Virtual address to unmap
 * @param size Size of the region to unmap
 * @return 0 on success, non-zero on failure
 * 
 * @author Fedi Nabli
 * @date 14 Mar 2025
 */
int mmu_unmap(uint64_t vaddr, uint64_t size);

#endif
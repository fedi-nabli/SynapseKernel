#ifndef __SYNAPSE_MEMORY_PAGING_H_
#define __SYNAPSE_MEMORY_PAGING_H_

#include <synapse/bool.h>
#include <synapse/types.h>

/* Page status flags */
#define PAGE_FREE       0x00
#define PAGE_RESERVED   0x01
#define PAGE_ALLOCATED  0X02
#define PAGE_MAPPED     0x04
#define PAGE_KERNEL     0x08
#define PAGE_ZEROED     0x10
#define PAGE_ACCESSED   0x20
#define PAGE_DIRTY      0x40

/* Page allocation flags */
#define PAGEF_ZEROED    0x01
#define PAGEF_KERNEL    0x02
#define PAGEF_CONT      0x04 // Contiguous pages

/**
 * @brief Initialize the paging subsystem
 * 
 * @param ram_size Total size of RAM in bytes
 * @param kernel_start Start address of the kernel
 * @param kernel_end End address of the kernel
 * @return int Status code (EOK on success, negative error code on failure)
 * 
 * @author Fedi Nabli
 * @date 19 Mar 2025
 */
int paging_init(size_t ram_size, uintptr_t kernel_start, uintptr_t kernel_end);

/**
 * @brief Allocate a single page of physical memory with flags
 * 
 * @param flags Allocation flags (e.g., PAGE_KERNEL, PAGE_ZEROED)
 * @return void* Pointer to the allocated page, or NULL if allocation fails
 * 
 * @author Fedi Nabli
 * @date 20 Mar 2025
 */
void* kpage_alloc_flags(uint32_t flags);

/**
 * @brief Allocate a single page of physical memory
 * 
 * @return void* Pointer to the allocated page, or NULL if allocation fails
 * 
 * @author Fedi Nabli
 * @date 20 Mar 2025
 */
void* kpage_alloc();

/**
 * @brief Allocate multiple contiguous pages
 * 
 * @param count Number of contiguous pages to allocate
 * @param flags Allocation flags (e.g., PAGE_KERNEL, PAGE_ZEROED)
 * @return void* Pointer to the start of the allocated pages, or NULL if allocation fails
 * 
 * @author Fedi Nabli
 * @date 20 Mar 2025
 */
void* kpage_alloc_contiguous(size_t count, uint32_t flags);

/**
 * @brief Free a previously allocated page
 * 
 * @param page Pointer to the page to free
 * @return int Status code (EOK on success, negative error code on failure)
 * 
 * @author Fedi Nabli
 * @date 20 Mar 2025
 */
int kpage_free(void* page);

/**
 * @brief Free multiple contiguous pages
 * 
 * @param page Pointer to the start of the pages to free
 * @param count Number of contiguous pages to free
 * @return int Status code (EOK on success, negative error code on failure)
 * 
 * @author Fedi Nabli
 * @date 20 Mar 2025
 */
int kpage_free_contiguous(void* page, size_t count);

/**
 * @brief Check if a page is allocated
 * 
 * @param page Pointer to the page to check
 * @return true if the page is allocated, false otherwise
 * 
 * @author Fedi Nabli
 * @date 20 Mar 2025
 */
bool kpage_is_allocated(void* page);

/**
 * @brief Get the physical address from a page pointer
 * 
 * @param page Pointer to the page
 * @return uintptr_t Physical address of the page
 * 
 * @author Fedi Nabli
 * @date 20 Mar 2025
 */
uintptr_t kpage_to_phys(void* page);

/**
 * @brief Get a page pointer from a physical address
 * 
 * @param phys Physical address of the page
 * @return void* Pointer to the page
 * 
 * @author Fedi Nabli
 * @date 20 Mar 2025
 */
void* kpage_from_phys(uintptr_t phys);

/**
 * @brief Handle a page fault exception
 * 
 * @param fault_addr Address where the page fault occurred
 * @param fault_status Status code of the fault
 * @return int Status code (EOK on success, negative error code on failure)
 * 
 * @author Fedi Nabli
 * @date 20 Mar 2025
 */
int kpage_fault_handler(uintptr_t fault_addr, uint64_t fault_status);

/**
 * @brief Get the total number of pages managed by the paging subsystem
 * 
 * @return size_t Total number of pages
 * 
 * @author Fedi Nabli
 * @date 20 Mar 2025
 */
size_t kpage_get_total();

/**
 * @brief Get the number of free pages managed by the paging subsystem
 * 
 * @return size_t Number of free pages
 * 
 * @author Fedi Nabli
 * @date 20 Mar 2025
 */
size_t kpage_get_free();

/**
 * @brief Get the number of used pages managed by the paging subsystem
 * 
 * @return size_t Number of used pages
 * 
 * @author Fedi Nabli
 * @date 20 Mar 2025
 */
size_t kpage_get_used();

/**
 * @brief Print page allocation statistics to UART
 * 
 * This function prints the total number of pages, used pages, free pages,
 * used memory, and free memory managed by the paging subsystem.
 * 
 * @author Fedi Nabli
 * @date 20 Mar 2025
 */
void kpage_print_stats();

#endif
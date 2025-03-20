/*
 * kernel_mmu.c - This file implements different kernel mmu
 * functions for memory management
 *
 * Author: Fedi Nabli
 * Date: 19 Mar 2025
 * Last Modified: 20 Mar 2025
 */

#include "kernel_mmu.h"

#include <uart.h>
#include <arm_mmu.h>

#include <synapse/types.h>
#include <synapse/status.h>

#include <synapse/memory/memory.h>
#include <synapse/memory/heap/kheap.h>
#include <synapse/memory/paging/paging.h>

// Page table pointers
static uint64_t* kernel_pgd = NULL; // Page Global directory
static uintptr_t kernel_pgd_phys = 0; // Physical address of kernel PGD

// Temporary buffer for early boot string operations
static char temp_str_buffer[32];

// Convert a number to a string for UART output
static char* uint_to_str(uint64_t value)
{
  int i = 0;
  char* p = temp_str_buffer;

  do
  {
    temp_str_buffer[i++] = '0' + (value % 10);
    value /= 10;
  } while (value > 0 && i < 31);
  
  temp_str_buffer[i] = '\0';

  // Reverse the string
  int j = 0;
  i--;
  while (j < i)
  {
    char temp = p[j];
    p[j] = p[i];
    p[i] = temp;
    j++;
    i--;
  }

  return temp_str_buffer;
}

/**
 * @brief Convert mapping flags to page table entries
 * 
 * @param flags flags to convert
 * @return uint64_t PTE attributes
 * 
 * @author Fedi Nabli
 * @date 19 Mar 2025
 */
static uint64_t flags_to_pte_attr(uint32_t flags)
{
  uint64_t attr = PTE_ATTR_AF; // Access flag always set

  // Apply memory types
  if (flags & MAP_DEVICE)
  {
    attr |= PTE_ATTR_ATTR_INDX(MEMORY_ATTR_DEVICE_nGnRE);
  }
  else if (flags & MAP_CACHE_WB)
  {
    attr |= PTE_ATTR_ATTR_INDX(MEMORY_ATTR_NORMAL_WB);
    attr |= PTE_ATTR_SH_INNER; // Inner shareable for normal memory
  }
  else if (flags & MAP_CACHE_WT)
  {
    attr |= PTE_ATTR_ATTR_INDX(MEMORY_ATTR_NORMAL_WT);
    attr |= PTE_ATTR_SH_INNER;
  }
  else if (flags & MAP_CACHE_NC)
  {
    attr |= PTE_ATTR_ATTR_INDX(MEMORY_ATTR_NORMAL_NC);
    attr |= PTE_ATTR_SH_INNER;
  }
  else
  {
    // Default to write-back memory
    attr |= PTE_ATTR_ATTR_INDX(MEMORY_ATTR_NORMAL_WB);
    attr |= PTE_ATTR_SH_INNER;
  }

  // Apply access permissions
  if (flags & MAP_WRITE)
  {
    if (flags & MAP_USER)
    {
      attr |= PTE_ATTR_AP_RW_ALL; // RW at all levels
    }
    else
    {
      attr |= PTE_ATTR_AP_RW_EL1; // RW at EL1, no access at EL0
    }
  }
  else
  {
    if (flags & MAP_USER)
    {
      attr |= PTE_ATTR_AP_RO_ALL; // RO at all levels
    }
    else
    {
      attr |= PTE_ATTR_AP_RO_EL1; // RO at EL1, no access at EL0
    }
  }

  // Apply execute permissions
  if (!(flags & MAP_EXEC))
  {
    attr |= PTE_ATTR_UXN; // User Execute Never
    attr |= PTE_ATTR_PXN; // Priviliged Execute Never
  }
  else if (!(flags & MAP_USER))
  {
    attr |= PTE_ATTR_UXN; // User Execute Never
  }

  return attr;
}

/**
 * @brief Allocate a page table
 * 
 * @return uint64_t* The allocated page table
 * 
 * @author Fedi Nabli
 * @date 19 Mar 2025
 */
static uint64_t* alloc_page_table()
{
  uint64_t* table = (uint64_t*)kpage_alloc();
  if (table)
  {
    memset(table, 0, PAGE_SIZE);
  }

  return table;
}

/**
 * @brief Convert a kernel virtual address to a physical address
 * 
 * @param virt The virtual address to convert
 * @return uintptr_t The physical address of a kernel virtual address
 */
static uintptr_t kernel_virt_to_phys(uintptr_t virt)
{
  // Direct convertion from kernel virtual to physical
  // This is a simplification, real implementation would use page tables
  return virt - KERNEL_VIRT_BASE;
}

/**
 * @brief Initialize the kernel MMU
 * 
 * @param ram_size Size of available RAM
 * @return int 0 if successful, non-zero on failure
 * 
 * @author Fedi Nabli
 * @date 19 Mar 2025
 */
int kernel_mmu_init(size_t ram_size)
{
  uart_send_string("Initializing kernel MMU...\n");

  // Allocate top-level page directory
  kernel_pgd = alloc_page_table();
  if (!kernel_pgd)
  {
    uart_send_string("Failed to allocate kernel PGD\n");
    return -ENOMEM;
  }

  // Get physical address of PGD
  kernel_pgd_phys = kernel_virt_to_phys((uintptr_t)kernel_pgd);

  // Configure MMU registers
  configure_mair_el1();
  configure_tcr_el1();
  configure_sctlr_el1();

  // Set translation base registers
  write_ttbr0_el1(0); // User space (will be setup later)
  write_ttbr1_el1(kernel_pgd_phys); // kernel space

  uart_send_string("Kernel MMU initialized with PGD at 0x");
  uart_send_string(uint_to_str(kernel_pgd_phys));
  uart_send_string("\n");
  
  return EOK;
}

/**
 * @brief Enable the MMU
 * 
 * @return int 0 if successful, non-zero on failure
 * 
 * @author Fedi Nabli
 * @date 19 Mar 2025
 */
int kernel_mmu_enable()
{
  uart_send_string("Enabling MMU...\n");

  // Read current SCTLR_EL1 value
  uint64_t sctlr = read_sctlr_el1();

  // Set the MMU Enable bit
  sctlr |= SCTLR_EL1_M;

  // Write back to SCTLR_EL1
  write_sctlr_el1(sctlr);

  dsb_sy();
  isb_sy();

  // Verify it's all set
  sctlr = read_sctlr_el1();
  if (!(sctlr & SCTLR_EL1_M))
  {
    uart_send_string("ERROR: Cannot enable MMU\n");
    return -EMMU;
  }

  uart_send_string("MMU enabled\n");
  return EOK;
}

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
int kernel_mmu_map(uintptr_t phys_addr, uintptr_t virt_addr, size_t size, uint32_t flags)
{
  // Align addresses and sizes to page boundaries
  uintptr_t phys_aligned = phys_addr & ~(PAGE_SIZE - 1);
  uintptr_t virt_aligned = virt_addr & ~(PAGE_SIZE - 1);
  size_t aligned_size = ((size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1));
  size_t pages = aligned_size / PAGE_SIZE;

  // Convert flags to page table attributes
  uint64_t pte_attr = flags_to_pte_attr(flags);

  // Iterate through each page
  for (size_t i = 0; i < pages; i++)
  {
    uintptr_t curr_virt = virt_aligned + (i * PAGE_SIZE);
    uintptr_t curr_phys = phys_aligned + (i * PAGE_SIZE);

    // Extract page table indices
    uint64_t pgd_index = (curr_virt >> PGD_SHIFT) & PGD_MASK;
    uint64_t pud_index = (curr_virt >> PUD_SHIFT) & PUD_MASK;
    uint64_t pmd_index = (curr_virt >> PMD_SHIFT) & PMD_MASK;
    uint64_t pte_index = (curr_phys >> PTE_SHIFT) & PTE_MASK;

    // Get or create the page upper directory (PUD)
    uint64_t pgd_entry = kernel_pgd[pgd_index];
    uint64_t* pud;

    if (!(pgd_entry & PTE_TYPE_TABLE))
    {
      // PUD doesn't exist, allocate it
      pud = alloc_page_table();
      if (!pud)
      {
        return -ENOMEM;
      }

      // Update PGD entry to point to new PUD
      kernel_pgd[pgd_index] = kernel_virt_to_phys((uintptr_t)pud) | PTE_TYPE_TABLE;
    }
    else
    {
      // PUD exists, get its address
      pud = (uint64_t*)((pgd_entry & PTE_TABLE_ADDR_MASK) + KERNEL_VIRT_BASE);
    }

    // Get or create the page middle directory (PMD)
    uint64_t pud_entry = pud[pud_index];
    uint64_t* pmd;

    if (!(pud_entry & PTE_TYPE_TABLE))
    {
      // PMD doesn't exist, allocate it
      pmd = alloc_page_table();
      if (!pmd)
      {
        return -ENOMEM;
      }

      // Update PUD entry to point to new PMD
      pud[pud_index] = kernel_virt_to_phys((uintptr_t)pmd) | PTE_TYPE_TABLE;
    }
    else
    {
      // PDM exists, get its address
      pmd = (uint64_t*)((pud_entry & PTE_TABLE_ADDR_MASK) + KERNEL_VIRT_BASE);
    }

    // Get or create the page table (PT)
    uint64_t pmd_entry = pmd[pmd_index];
    uint64_t* pt;

    if (!(pmd_entry & PTE_TYPE_TABLE))
    {
      // PT deosn't exist, allocate it
      pt = alloc_page_table();
      if (!pt)
      {
        return -ENOMEM;
      }

      // Update PMD entry to point to new PT
      pmd[pmd_index] = kernel_virt_to_phys((uintptr_t)pt) | PTE_TYPE_TABLE;
    }
    else
    {
      // PT exists, get its address
      pt = (uint64_t*)((pmd_entry & PTE_TABLE_ADDR_MASK) + KERNEL_VIRT_BASE);
    }

    // Create PTE to map the physical base
    pt[pte_index] = (curr_phys & PTE_BLOCK_ADDR_MASK) | pte_attr | PTE_TYPE_PAGE;
  }

  // Invalidate TLB entries for the mapped range
  for (size_t i = 0; i < pages; i++)
  {
    invalidate_tlb_entry(virt_aligned + (i * PAGE_SIZE));
  }

  return EOK;
}

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
int kernel_mmu_identity_map(uintptr_t phys_addr, size_t size, uint32_t flags)
{
  return kernel_mmu_map(phys_addr, phys_addr, size, flags);
}

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
int kernel_mmu_unmap(uintptr_t virt_addr, size_t size)
{
  // Align addresses and sizes to page boundaries
  uintptr_t virt_aligned = virt_addr & ~(PAGE_SIZE - 1);
  size_t aligned_size = ((size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1));
  size_t pages = aligned_size / PAGE_SIZE;

  // Iterate through each page
  for (size_t i = 0; i < pages; i++)
  {
    uintptr_t curr_virt = virt_aligned + (i * PAGE_SIZE);

    // Extract page table indices
    uint64_t pgd_index = (curr_virt >> PGD_SHIFT) & PGD_MASK;
    uint64_t pud_index = (curr_virt >> PUD_SHIFT) & PUD_MASK;
    uint64_t pmd_index = (curr_virt >> PMD_SHIFT) & PMD_MASK;
    uint64_t pte_index = (curr_virt >> PTE_SHIFT) & PTE_MASK;

    // Check if thr PGD entry exists
    uint64_t pgd_entry = kernel_pgd[pgd_index];
    if (!(pgd_entry & PTE_TYPE_TABLE))
    {
      continue; // Skip if PGD entry doesn't exist
    }

    // Get PUD
    uint64_t* pud = (uint64_t*)((pgd_entry & PTE_TABLE_ADDR_MASK) + KERNEL_VIRT_BASE);
    uint64_t pud_entry = pud[pud_index];
    if (!(pud_entry & PTE_TYPE_TABLE))
    {
      continue; // Skip if PUD entry doesn't exist
    }

    // Get PMD
    uint64_t* pmd = (uint64_t*)((pud_entry & PTE_TABLE_ADDR_MASK) + KERNEL_VIRT_BASE);
    uint64_t pmd_entry = pmd[pmd_index];
    if (!(pmd_entry & PTE_TYPE_TABLE))
    {
      continue; // Skip if PMD entry doesn't exist
    }

    // Get PT
    uint64_t* pt = (uint64_t*)((pmd_entry & PTE_TABLE_ADDR_MASK) + KERNEL_VIRT_BASE);

    // Clear the PTE
    pt[pte_index] = 0;

    // Invalidate TLB entry for this page
    invalidate_tlb_entry(curr_virt);
  }

  return EOK;
}

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
int kernel_mmu_translate(uintptr_t virt_addr, uintptr_t* phys_addr)
{
  if (!phys_addr)
  {
    return -EINVARG;
  }

  // Extract page table indices
  uint64_t pgd_index = (virt_addr >> PGD_SHIFT) & PGD_MASK;
  uint64_t pud_index = (virt_addr >> PUD_SHIFT) & PUD_MASK;
  uint64_t pmd_index = (virt_addr >> PMD_SHIFT) & PMD_MASK;
  uint64_t pte_index = (virt_addr >> PTE_SHIFT) & PTE_MASK;

  // Walk the page tables
  uint64_t pgd_entry = kernel_pgd[pgd_index];
  if (!(pgd_entry & PTE_TYPE_TABLE))
  {
    return -EFAULT; // Address not mapped
  }

  uint64_t* pud = (uint64_t*)((pgd_entry & PTE_TABLE_ADDR_MASK) + KERNEL_VIRT_BASE);
  uint64_t pud_entry = pud[pud_index];
  if (!(pud_entry & PTE_TYPE_TABLE))
  {
    return -EFAULT; // Address not mapped
  }

  uint64_t* pmd = (uint64_t*)((pud_entry & PTE_TABLE_ADDR_MASK) + KERNEL_VIRT_BASE);
  uint64_t pmd_entry = pmd[pmd_index];
  if (!(pmd_entry & PTE_TYPE_TABLE))
  {
    return -EFAULT; // Address not mapped
  }

  uint64_t* pt = (uint64_t*)((pmd_entry & PTE_TABLE_ADDR_MASK) + KERNEL_VIRT_BASE);
  uint64_t pte = pt[pte_index];
  if (!(pte & PTE_TYPE_PAGE))
  {
    return -EFAULT; // Address not mapped
  }

  // Calculate the physical address
  *phys_addr = (pte & PTE_BLOCK_ADDR_MASK) | (virt_addr & (PAGE_SIZE - 1));

  return EOK;
}

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
int kernel_mmu_get_flags(uintptr_t virt_addr, uint32_t* flags)
{
  if (!flags)
  {
    return -EINVARG;
  }

  // Extract page table indices
  uint64_t pgd_index = (virt_addr >> PGD_SHIFT) & PGD_MASK;
  uint64_t pud_index = (virt_addr >> PUD_SHIFT) & PUD_MASK;
  uint64_t pmd_index = (virt_addr >> PMD_SHIFT) & PMD_MASK;
  uint64_t pte_index = (virt_addr >> PTE_SHIFT) & PTE_MASK;

  // Walk the page tables
  uint64_t pgd_entry = kernel_pgd[pgd_index];
  if (!(pgd_entry & PTE_TYPE_TABLE))
  {
    return -EFAULT; // Address not mapped
  }

  uint64_t* pud = (uint64_t*)((pgd_entry & PTE_TABLE_ADDR_MASK) + KERNEL_VIRT_BASE);
  uint64_t pud_entry = pud[pud_index];
  if (!(pud_entry & PTE_TYPE_TABLE))
  {
    return -EFAULT; // Address not mapped
  }

  uint64_t* pmd = (uint64_t*)((pud_entry & PTE_TABLE_ADDR_MASK) + KERNEL_VIRT_BASE);
  uint64_t pmd_entry = pmd[pmd_index];
  if (!(pmd_entry & PTE_TYPE_TABLE))
  {
    return -EFAULT; // Address not mapped
  }

  uint64_t* pt = (uint64_t*)((pmd_entry & PTE_TABLE_ADDR_MASK) + KERNEL_VIRT_BASE);
  uint64_t pte = pt[pte_index];
  if (!(pte & PTE_TYPE_PAGE))
  {
    return -EFAULT; // Address not mapped
  }

  // Convert page table attributes to flags
  *flags = 0;

  // Extract memory type
  uint64_t attr_idx = (pte >> 2) & 0x7;
  if (attr_idx == MEMORY_ATTR_DEVICE_nGnRE ||
      attr_idx == MEMORY_ATTR_DEVICE_nGnRnE ||
      attr_idx == MEMORY_ATTR_DEVICE_GRE)
  {
    *flags |= MAP_DEVICE;
  }
  else if (attr_idx == MEMORY_ATTR_NORMAL_WB)
  {
    *flags |= MAP_CACHE_WB;
  }
  else if (attr_idx == MEMORY_ATTR_NORMAL_WT)
  {
    *flags |= MAP_CACHE_WT;
  }
  else if (attr_idx |= MEMORY_ATTR_NORMAL_NC)
  {
    *flags |= MAP_CACHE_NC;
  }

  // Extract access permissions
  uint64_t ap = (pte >> 6) & 0x3;
  if (ap == 0)
  {
    *flags |= MAP_WRITE; // RW at EL1
  }
  else if (ap == 1)
  {
    *flags |= MAP_WRITE | MAP_USER; // RW at all levels
  }
  else if (ap == 2)
  {
    *flags |= 0; // RO at EL1
  }
  else if (ap == 3)
  {
    *flags |= MAP_USER; // RO at all levels
  }

  // Extract execute permissions
  if (!(pte & PTE_ATTR_UXN) && !(pte & PTE_ATTR_PXN))
  {
    *flags |= MAP_EXEC; // Executable at all levels
  }
  else if (!(pte & PTE_ATTR_PXN) && (pte & PTE_ATTR_UXN))
  {
    *flags |= MAP_EXEC; // Executable at EL1 only
  }

  return EOK;
}

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
int kernel_mmu_set_flags(uintptr_t virt_addr, uint32_t flags)
{
  uintptr_t phys_addr;
  int res = kernel_mmu_translate(virt_addr, &phys_addr);
  if (res < 0)
  {
    return res;
  }

  // Unmap and remap with new flags
  kernel_mmu_unmap(virt_addr, PAGE_SIZE);
  return kernel_mmu_map(phys_addr, virt_addr, PAGE_SIZE, flags);
}

/**
 * @brief Print the MMU configuration for debugging
 * 
 * @author Fedi Nabli
 * @date 20 Mar 2025
 */
void kernel_mmu_print_config()
{
  uart_send_string("MMU Configuration:\n");
  uart_send_string("  PGD Physical: 0x");
  uart_send_string(uint_to_str(kernel_pgd_phys));
  uart_send_string("\n  SCTLR_EL1: 0x");
  uart_send_string(uint_to_str(read_sctlr_el1()));
  uart_send_string("\n  TCR_EL1: 0x");
  uart_send_string(uint_to_str(read_tcr_el1()));
  uart_send_string("\n  MAIR_EL1: 0x");
  uart_send_string(uint_to_str(read_mair_el1()));
  uart_send_string("\n  TTBR0_EL1: 0x");
  uart_send_string(uint_to_str(read_ttbr0_el1()));
  uart_send_string("\n  TTBR1_EL1: 0x");
  uart_send_string(uint_to_str(read_ttbr1_el1()));
  uart_send_string("\n");
}

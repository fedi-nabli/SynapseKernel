/*
 * paging.c - Paging Manager Implementation
 *
 * This file implements the paging system that provides
 * convenient abstractions over the MMU operations
 * 
 * Author: Fedi Nabli
 * Date: 16 Mar 2025
 * Last Modified: 16 Mar 2025
 */

#include "paging.h"

#include <synapse/bool.h>
#include <synapse/types.h>
#include <synapse/status.h>

#include <uart.h>
#include <arm_mmu.h>
#include <kernel/mmu.h>
#include <synapse/memory/heap/kheap.h>

#define MAX_ALLOCATIONS 128
#define CACHE_LINE_SIZE 64 // 64-byte cache line size

/* Region definitions */
typedef struct
{
  uint64_t start_addr; // Start virtual address
  uint64_t end_addr;  // End virtual address
  uint64_t current; // Current allocation pointer
  uint32_t default_flags; // Default access flags
} region_info_t;

/* Allocation tracking */
typedef struct 
{
  uint64_t virt_addr; // Virtual address
  uint64_t phys_addr; // Physical address
  size_t size; // Size of allocation
  bool used; // Wheather entry is in use
  memory_region_t region; // Region this allocation belongs to
} allocation_info_t;


/* Region definitions */
static region_info_t regions[REGION_MAX] = {
  [REGION_KERNEL_CODE]    = { 0xFFFFFFFF80000000ULL, 0xFFFFFFFF9FFFFFFFULL, 0, PAGE_KERNEL_CODE },
  [REGION_KERNEL_RODATA]  = { 0xFFFFFFFFA0000000ULL, 0xFFFFFFFFAFFFFFFFULL, 0, PAGE_KERNEL_RO },
  [REGION_KERNEL_DATA]    = { 0xFFFFFFFFB0000000ULL, 0xFFFFFFFFBFFFFFFFULL, 0, PAGE_KERNEL_RW },
  [REGION_KERNEL_HEAP]    = { 0xFFFFFFFFC0000000ULL, 0xFFFFFFFFCFFFFFFFULL, 0, PAGE_KERNEL_RW },
  [REGION_KERNEL_STACK]   = { 0xFFFFFFFFD0000000ULL, 0xFFFFFFFFDFFFFFFFULL, 0, PAGE_KERNEL_RW },
  [REGION_USER_CODE]      = { 0x0000000001000000ULL, 0x00000000FFFFFFFFULL, 0, PAGE_USER_CODE },
  [REGION_USER_DATA]      = { 0x0000000100000000ULL, 0x00000001FFFFFFFFULL, 0, PAGE_USER_RW },
  [REGION_USER_HEAP]      = { 0x0000000200000000ULL, 0x00000002FFFFFFFFULL, 0, PAGE_USER_RW },
  [REGION_USER_STACK]     = { 0x0000000300000000ULL, 0x00000003FFFFFFFFULL, 0, PAGE_USER_RW },
  [REGION_DEVICE]         = { 0xFFFFFFFF00000000ULL, 0xFFFFFFFF7FFFFFFFULL, 0, PAGE_DEVICE_MEM },
  [REGION_AI_TENSOR]      = { 0xFFFFFFFFE0000000ULL, 0xFFFFFFFFEFFFFFFFULL, 0, PAGE_KERNEL_RW | PAGE_SHARED }
};

/* Allocation tracking */
static allocation_info_t allocations[MAX_ALLOCATIONS];
static bool initialized = false;

static uint64_t flags_to_mmu_attrs(uint32_t flags)
{
  uint64_t attrs = PTE_ATTR_AF; // Always set access flag

  // Memory attributes
  if (flags & PAGE_DEVICE)
  {
    attrs |= PTE_ATTR_ATTR_INDX(MEMORY_ATTR_DEVICE_nGnRE);
  }
  else if (flags & PAGE_NOCACHE)
  {
    attrs |= PTE_ATTR_ATTR_INDX(MEMORY_ATTR_NORMAL_NC);
  }
  else
  {
    attrs |= PTE_ATTR_ATTR_INDX(MEMORY_ATTR_NORMAL_WB);
  }

  // Access permissions
  if (flags & PAGE_WRITE)
  {
    if (flags & PAGE_USER)
    {
      attrs |= PTE_ATTR_AP_RW_ALL;
    }
    else
    {
      attrs |= PTE_ATTR_AP_RW_EL1;
    }
  }
  else
  {
    if (flags & PAGE_USER)
    {
      attrs |= PTE_ATTR_AP_RO_ALL;
    }
    else
    {
      attrs |= PTE_ATTR_AP_RO_EL1;
    }
  }

  // Execute permissions
  if (!(flags & PAGE_EXEC))
  {
    attrs |= PTE_ATTR_UXN | PTE_ATTR_PXN;
  }
  else if (!(flags & PAGE_USER))
  {
    attrs |= PTE_ATTR_UXN; // Executable only at EL1
  }

  // Shareability
  if (flags & PAGE_SHARED)
  {
    attrs |= PTE_ATTR_SH_INNER;
  }

  return attrs;
}

/**
 * @brief Find a free allocation entry
 * 
 * @return int Index of free entry, or -1 if none
 * 
 * @author Fedi Nabli
 * @date 17 Mar 2025
 */
static int find_free_allocation()
{
  for (int i = 0; i < MAX_ALLOCATIONS; i++)
  {
    if (!allocations[i].used)
    {
      return i;
    }
  }

  return -1;
}

/**
 * @brief Find alocation info for a virtual address
 * 
 * @param vaddr Virtual address to look up
 * @return int Index of allocation entry, or -1 if not found
 * 
 * @author Fedi Nabli
 * @date 17 Mar 2025
 */
static int find_allocation(uint64_t vaddr)
{
  for (int i = 0; i < MAX_ALLOCATIONS; i++)
  {
    if (allocations[i].used && allocations[i].virt_addr == vaddr)
    {
      return i;
    }
  }

  return -1;
}

/**
 * @brief Allocate a physical memory chunk
 * 
 * @param size Sizes in byte
 * @return Physical address, or 0 on failure
 * 
 * @author Fedi Nabli
 * @date 17 Mar 2025
 */
static uint64_t alloc_physical(size_t size)
{
  // Round up to page size
  size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

  // For now, just use kmalloc
  // In a more sophisticated implementation, we would use a
  // dedicated physical memory allocator
  void* ptr = kmalloc(size);
  if (!ptr)
  {
    return 0;
  }

  return (uint64_t)ptr;
}

/**
 * @brief Free physical memory
 * 
 * @param paddr Physical
 * 
 * @author Fedi Nabli
 * @date 17 Mar 2025
 */
static void free_physical(uint64_t paddr)
{
  // For now, just use kree
  kfree((void*)paddr);
}

/**
 * @brief Allocate virtual address space from a region
 * 
 * @param region Memory region
 * @param size Size needed
 * @param alignment Alignment requirement (power of 2)
 * @return Virtual address, or 0 on failure
 * 
 * @author Fedi Nabli
 * @date 17 Mar 2025
 */
static uint64_t alloc_virtual(memory_region_t region, size_t size, size_t alignment)
{
  if (region >= REGION_MAX)
  {
    return 0;
  }

  // Round up to page size
  size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

  // Ensure alignment is at least page size and power of 2
  if (alignment < PAGE_SIZE || (alignment & (alignment - 1)) != 0)
  {
    alignment = PAGE_SIZE;
  }

  // Align current pointer
  uint64_t current = (regions[region].current + alignment - 1) & ~(alignment - 1);

  // Check if we have enough space in this region
  if (current + size > regions[region].end_addr)
  {
    uart_send_string("ERROR: Region out of space\n");
    return 0;
  }

  // Reserve the space
  uint64_t result = current;
  regions[region].current = current + size;

  return result;
}

/**
 * @brief Initialize the paging system
 * 
 * @author Fedi Nabli
 * @date 16 Mar 2025
 */
int paging_init()
{
  uart_send_string("Initializing paging system...\n");

  // Initialize current pointers for each region
  for (int i = 0; i < REGION_MAX; i++)
  {
    regions[i].current = regions[i].start_addr;
  }

  // Clear allocation tracking
  for (int i = 0; i <MAX_ALLOCATIONS; i++)
  {
    allocations[i].virt_addr = 0;
    allocations[i].phys_addr = 0;
    allocations[i].size = 0;
    allocations[i].used = false;
  }

  initialized = true;
  uart_send_string("Paging system initialized\n");

  return EOK;
}

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
int paging_map(uint64_t vaddr, uint64_t paddr, size_t size, uint32_t flags)
{
  if (!initialized)
  {
    int res = paging_init();
    if (res != EOK)
    {
      return res;
    }
  }

  // Ensure addresses are page-aligned
  if ((vaddr & (PAGE_SIZE - 1)) || (paddr & (PAGE_SIZE -1)))
  {
    uart_send_string("ERROR: Addresses must be page-aligned\n");
    return -EINVAL;
  }

  // Convert flags to MMU attributes
  uint64_t mmu_attrs = flags_to_mmu_attrs(flags);

  // Call the MMU mapping function
  return mmu_map(vaddr, paddr, size, mmu_attrs);
}

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
int paging_unmap(uint64_t vaddr, size_t size)
{
  if (!initialized)
  {
    return -ENOTREADY;
  }

  // Call the MMU unmapping function
  return mmu_unmap(vaddr, size);
}

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
uint64_t paging_alloc_from_region(memory_region_t region, size_t size, uint32_t flags)
{
  if (!initialized)
  {
    int res = paging_init();
    if (res != EOK)
    {
      return 0;
    }
  }

  if (region >= REGION_MAX)
  {
    uart_send_string("ERROR: Invalid region\n");
    return 0;
  }

  // Find a free allocation entry
  int alloc_idx = find_free_allocation();
  if (alloc_idx < 0)
  {
    uart_send_string("ERROR: No free allocation entries\n");
    return 0;
  }

  // Allocate virtual address space
  uint64_t vaddr = alloc_virtual(region, size, PAGE_SIZE);
  if (!vaddr)
  {
    uart_send_string("ERROR: Failed to allocate virtual address space\n");
    return 0;
  }

  // ALlocate physical address
  uint64_t paddr = alloc_physical(size);
  if (!paddr)
  {
    uart_send_string("ERROR: Failed to allocate physical memory\n");
    return 0;
  }

  // If no flags specified, use region defaults
  if (flags == 0)
  {
    flags = regions[region].default_flags;
  }

  // Map the memory
  int res = paging_map(vaddr, paddr, size, flags);
  if (res != EOK)
  {
    free_physical(paddr);
    uart_send_string("ERROR: Failed to map memory\n");
    return 0;
  }

  // Track the allocation
  allocations[alloc_idx].virt_addr = vaddr;
  allocations[alloc_idx].phys_addr = paddr;
  allocations[alloc_idx].size = size;
  allocations[alloc_idx].used = true;
  allocations[alloc_idx].region = region;

  return vaddr;
}

/**
 * @brief Allocates a tensor in the paging memory system.
 * 
 * This function allocates a block of memory of the specified size and applies the given flags to it.
 * 
 * @param size The size of the memory block to allocate.
 * @param flags The flags to apply to the allocated memory block.
 * @return uint64_t The address of the allocated memory block.
 */
uint64_t paging_alloc_tensor(size_t size, uint32_t flags)
{
  // Ensure size if a multiple of cache line size for better performance
  size = (size + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1);

  // Add tensor-specific flags
  flags |= PAGE_READ | PAGE_WRITE | PAGE_SHARED;

  // Allocate from tensor region
  return paging_alloc_from_region(REGION_AI_TENSOR, size, flags);
}

/**
 * @brief Free memory allocated with paging_alloc functions
 * 
 * @param vaddr Virtual address of the memory to free
 * @return int 0 if successful, non-zero if failure
 * 
 * @author Fedi Nabli
 * @date 17 Mar 2025
 */
int paging_free(uint64_t vaddr)
{
  if (!initialized)
  {
    return -ENOTREADY;
  }

  // Find the allocation
  int idx = find_allocation(vaddr);
  if (idx < 0)
  {
    uart_send_string("ERROR: Allocation not found\n");
    return -EINVAL;
  }

  // Unmap the memory
  int res = paging_unmap(vaddr, allocations[idx].size);
  if (res != EOK)
  {
    uart_send_string("ERROR: Failed to unmap memory\n");
    return res;
  }

  // Free the physical memory
  free_physical(allocations[idx].phys_addr);

  // Clear the allocatoin entry
  allocations[idx].virt_addr = 0;
  allocations[idx].phys_addr = 0;
  allocations[idx].size = 0;
  allocations[idx].used = false;

  return EOK;
}

/**
 * @brief Get physical address for a virtual address
 * 
 * @param vaddr Virtual address to translate
 * @return uint64_t Physical address corresponding to the virtual address, or 0 if not mapped
 * 
 * @author Fedi Nabli
 * @date 17 Mar 2025
 */
uint64_t paging_get_phys_addr(uint64_t vaddr)
{
  // First check our tracking
  for (int i = 0; i < MAX_ALLOCATIONS; i++)
  {
    if (allocations[i].used &&
        vaddr >= allocations[i].virt_addr &&
        vaddr < allocations[i].virt_addr + allocations[i].size)
    {
      uint64_t offset = vaddr - allocations[i].virt_addr;
      return allocations[i].phys_addr + offset;
    }
  }

  // Fall back to MMU translation
  return mmu_virt_to_phys(vaddr);
}

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
int paging_set_protection(uint64_t vaddr, size_t size, uint32_t flags)
{
  if (!initialized)
  {
    return -ENOTREADY;
  }

  // This would normally remap the memory with new permissions
  // For simplicity, we'll first unmap and then remap

  // Find the allocation to get the physical address
  uint64_t paddr = paging_get_phys_addr(vaddr);
  if (!paddr)
  {
    uart_send_string("ERROR: Failed to get physical address\n");
    return -EINVAL;
  }

  // Unmap the memory
  int res = paging_unmap(vaddr, size);
  if (res != EOK)
  {
    uart_send_string("ERROR: Failed to unmap memory\n");
    return res;
  }

  // Remap with the new flags
  res = paging_map(vaddr, paddr, size, flags);
  if (res != EOK)
  {
    uart_send_string("ERROR: Failed to remap memory\n");
    return res;
  }

  return EOK;
}


/**
 * @brief Convert flags to string representations
 * 
 * @param flags Flags to convert
 * @param buffer Buffer to store the string representation
 * @return void
 * 
 * @author Fedi Nabli
 * @date 17 Mar 2025
 */
static void flags_to_string(uint32_t flags, char* buffer)
{
  int pos = 0;

  if (flags & PAGE_READ) buffer[pos++] = 'R';
  if (flags & PAGE_WRITE) buffer[pos++] = 'W';
  if (flags & PAGE_EXEC) buffer[pos++] = 'X';
  if (flags & PAGE_USER) buffer[pos++] = 'U';
  if (flags & PAGE_NOCACHE)
  {
    buffer[pos++] = 'N';
    buffer[pos++] = 'C';
  }
  if (flags & PAGE_DEVICE)
  {
    buffer[pos++] = 'D';
    buffer[pos++] = 'E';
    buffer[pos++] = 'V';
  }
  if (flags & PAGE_SHARED)
  {
    buffer[pos++] = 'S';
    buffer[pos++] = 'H';
  }

  buffer[pos++] = '\0';
}

/**
 * @brief Dump paging system information
 * 
 * @param vaddr Virtual address to dump information for, or 0 for summary
 * 
 * @author Fedi Nabli
 * @date 17 Mar 2025
 */
void paging_dump_info(uint64_t vaddr)
{
  if (!initialized)
  {
    uart_send_string("Paging system not initalized\n");
    return;
  }

  if (vaddr != 0)
  {
    // Dump info for a specific address
    uart_send_string("Address 0x");
    char hex[17] = "0000000000000000";
    for (int i = 0; i < 16; i++)
    {
      int digit = (vaddr >> ((15-i) * 4)) & 0xF;
      hex[i] = digit < 10 ? '0' + digit : 'A' + (digit - 10);
    }
    uart_send_string(hex);

    // Find which region it belongs to
    memory_region_t region = REGION_MAX;
    for (int i = 0; i < REGION_MAX; i++)
    {
      if (vaddr >= regions[i].start_addr && vaddr < regions[i].end_addr)
      {
        region = (memory_region_t)i;
        break;
      }
    }

    if (region != REGION_MAX)
    {
      uart_send_string("\nRegion: ");
      switch (region)
      {
        case REGION_KERNEL_CODE:  uart_send_string("KERNEL_CODE"); break;
        case REGION_KERNEL_RODATA:  uart_send_string("KERNEL_RODATA"); break;
        case REGION_KERNEL_DATA:  uart_send_string("KERNEL_DATA"); break;
        case REGION_KERNEL_HEAP:  uart_send_string("KERNEL_HEAP"); break;
        case REGION_KERNEL_STACK: uart_send_string("KERNEL_STACK"); break;
        case REGION_USER_CODE:  uart_send_string("USER_CODE"); break;
        case REGION_USER_DATA:  uart_send_string("USER_DATA"); break;
        case REGION_USER_HEAP:  uart_send_string("USER_HEAP"); break;
        case REGION_USER_STACK: uart_send_string("USER_STACK"); break;
        case REGION_DEVICE: uart_send_string("DEVICE"); break;
        case REGION_AI_TENSOR:  uart_send_string("AI_TENSOR"); break;
        default:  uart_send_string("UNKNOWN"); break;
      }
    }
    else
    {
      uart_send_string("\nRegion: UNKNOWN");
    }

    // Check if it's tracked in our allocations
    int idx = -1;
    for (int i = 0; i < MAX_ALLOCATIONS; i++)
    {
      if (allocations[i].used &&
          vaddr >= allocations[i].virt_addr &&
          vaddr < allocations[i].virt_addr + allocations[i].size)
      {
        idx = i;
        break;
      }
    }

    if (idx >= 0)
    {
      uart_send_string("\nTracked allocation:");
      uart_send_string("\n  VA: 0x");
      for (int i = 0; i < 16; i++)
      {
        int digit = (allocations[idx].virt_addr >> ((15-i) * 4)) & 0xF;
        hex[i] = digit < 10 ? '0' + digit : 'A' + (digit - 10);
      }
      uart_send_string(hex);

      uart_send_string("\n  PA: 0x");
      for (int i = 0; i < 16; i++)
      {
        int digit = (allocations[i].phys_addr >> ((15-i) * 4)) & 0xF;
        hex[i] = digit < 10 ? '0' + digit : 'A' + (digit - 10);
      }
      uart_send_string(hex);

      uart_send_string("\n  Size: ");
      char size_str[20];
      size_t size = allocations[idx].size;
      int pos = 0;

      if (size == 0)
      {
        size_str[pos++] = '0';
      }
      else
      {
        char digits[20];
        int digit_pos = 0;

        while (size > 0)
        {
          digits[digit_pos++] = '0' + (size % 10);
          size /= 10;
        }

        for (int i = digit_pos - 1; i >= 0; i--)
        {
          size_str[pos++] = digits[i];
        }
      }

      size_str[pos] = '\0';
      uart_send_string(size_str);
      uart_send_string(" bytes");
    }
    else
    {
      // Try MMU translation
      uint64_t phys = mmu_virt_to_phys(vaddr);

      if (phys)
      {
        uart_send_string("\nMMU mapping:");
        uart_send_string("\n  PA: 0x");
        for (int i = 0; i < 16; i++)
        {
          int digit = (phys >> ((15-i) * 4)) & 0xF;
          hex[i] = digit < 10 ? '0' + digit : 'A' + (digit - 10);
        }
        uart_send_string(hex);
      }
      else
      {
        uart_send_string("\nNot mapped");
      }
    }

    uart_send_string("\n");
  }
  else
  {
    // Dump summary
    uart_send_string("Paging System Summary:\n");
    uart_send_string("=====================\n");

    uart_send_string("Memory Regions:\n");
    for (int i = 0; i < REGION_MAX; i++)
    {
      switch (i)
      {
        case REGION_KERNEL_CODE:  uart_send_string("  KERNEL_CODE:   "); break;
        case REGION_KERNEL_RODATA:  uart_send_string("  KERNEL_RODATA: "); break;
        case REGION_KERNEL_DATA:  uart_send_string("  KERNEL_DATA:   "); break;
        case REGION_KERNEL_HEAP:  uart_send_string("  KERNEL_HEAP:   "); break;
        case REGION_KERNEL_STACK: uart_send_string("  KERNEL_STACK:  "); break;
        case REGION_USER_CODE:  uart_send_string("  USER_CODE:     "); break;
        case REGION_USER_DATA:  uart_send_string("  USER_DATA:     "); break;
        case REGION_USER_HEAP:  uart_send_string("  USER_HEAP:     "); break;
        case REGION_USER_STACK: uart_send_string("  USER_STACK:     "); break;
        case REGION_DEVICE: uart_send_string("  DEVICE:        "); break;
        case REGION_AI_TENSOR:  uart_send_string("  AI_TENSOR     "); break;
        default: uart_send_string("  UNKNOWN:       "); break;
      }

      // Print usage percentage
      uint64_t total_size = regions[i].end_addr - regions[i].start_addr;
      uint64_t used_size = regions[i].current - regions[i].start_addr;
      uint64_t percent = (used_size * 100) / total_size;

      char percent_str[5];
      int pos = 0;

      if (percent == 0)
      {
        percent_str[pos++] = '0';
      }
      else
      {
        char digits[5];
        int digit_pos = 0;

        while (percent > 0 && digit_pos < 3)
        {
          digits[digit_pos++] = '0' + (percent % 10);
          percent /= 10;
        }

        for (int j = digit_pos - 1; j >= 0; j--)
        {
          percent_str[pos++] = digits[j];
        }
      }

      percent_str[pos] = '\0';
      uart_send_string(percent_str);
      uart_send_string("% used (");

      // Print flags
      char flags_str[10];
      flags_to_string(regions[i].default_flags, flags_str);
      uart_send_string(flags_str);
      uart_send_string(")\n");
    }

    uart_send_string("\nActive Allocations: ");

    int count = 0;
    for (int i = 0; i < MAX_ALLOCATIONS; i++)
    {
      if (allocations[i].used)
      {
        count++;
      }
    }

    char count_str[10];
    int pos = 0;

    if (count == 0)
    {
      count_str[pos++] = '0';
    }
    else
    {
      char digits[10];
      int digit_pos = 0;

      while (count > 0)
      {
        digits[digit_pos++] = '0' + (count % 10);
        count /= 10;
      }

      for (int i = digit_pos - 1; i >= 0; i--)
      {
        count_str[pos++] = digits[i];
      }
    }

    count_str[pos] = '\0';
    uart_send_string(count_str);
    uart_send_string("\n");
  }
}

#include "memory_system.h"

#include <uart.h>

#include <synapse/bool.h>
#include <synapse/types.h>
#include <synapse/status.h>

#include <kernel/config.h>
#include <kernel/kernel_mmu.h>

#include <synapse/memory/memory.h>
#include <synapse/memory/heap/kheap.h>
#include <synapse/memory/paging/paging.h>
#include <synapse/memory/ai_memory/ai_memory.h>

// Global memory regions array
static mem_system_region_t memory_regions[MAX_MEMORY_REGIONS];
static size_t num_memory_regions = 0;

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
 * @brief Helper function to copy a string with length limit
 * 
 * @param dest Destination buffer where the string will be copied
 * @param src Source string to be copied
 * @param max_len Maximum length of the destination buffer
 * 
 * @author Fedi Nabli
 * @date 21 Mar 2025
 */
static void str_copy(char* dest, const char* src, size_t max_len)
{
  size_t i;
  for (i = 0; i < max_len - 1 && src[i] != '\0'; i++)
  {
    dest[i] = src[i];
  }
  dest[i] = '\0';
}

/**
 * @brief Add a memory region to the tracking array
 * 
 * This function adds a memory region to the global memory regions array.
 * It ensures that the number of memory regions does not exceed the maximum limit.
 * The function copies the provided name to the region's name field using a safe string copy function.
 * 
 * @param phys_start The physical start address of the memory region
 * @param virt_start The virtual start address of the memory region
 * @param size The size of the memory region
 * @param type The type of the memory region
 * @param name The name of the memory region
 * @return int Returns EOK on success, or -ENOMEM if the maximum number of regions is exceeded
 * 
 * @author Fedi Nabli
 * @date 21 Mar 2025
 */
static int add_memory_region(uintptr_t phys_start, uintptr_t virt_start, size_t size, mem_region_type_t type, const char* name)
{
  if (num_memory_regions >= MAX_MEMORY_REGIONS)
  {
    return -ENOMEM;
  }

  mem_system_region_t* region = &memory_regions[num_memory_regions];

  region->phys_start = phys_start;
  region->phys_end = phys_start + size;
  region->virt_start = virt_start;
  region->size = size;
  region->type = type;
  str_copy(region->name, name, sizeof(region->name));

  num_memory_regions++;

  return EOK;
}

/**
 * @brief Convert memory region type to mapping flags
 * 
 * This function converts a memory region type to the corresponding mapping flags.
 * The mapping flags are used to configure the memory management unit (MMU) for the region.
 * 
 * @param type The type of the memory region
 * @return uint32_t The mapping flags for the given memory region type
 * 
 * @author Fedi Nabli
 * @date 21 Mar 2025
 */
// static uint32_t region_type_to_flags(mem_region_type_t type)
// {
//   switch (type)
//   {
//     case MEM_REGION_TYPE_RAM:
//       return MAP_READ | MAP_WRITE | MAP_CACHE_WB;
//     case MEM_REGION_TYPE_DEVICE:
//       return MAP_READ | MAP_WRITE | MAP_DEVICE;
//     case MEM_REGION_TYPE_MMIO:
//       return MAP_READ | MAP_WRITE | MAP_DEVICE;
//     case MEM_REGION_TYPE_KERNEL:
//       return MAP_READ | MAP_WRITE | MAP_EXEC | MAP_CACHE_WB;
//     default:
//       return MAP_READ | MAP_CACHE_WB;
//   }
// }

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
int memory_system_init(size_t ram_size, uintptr_t kernel_start, uintptr_t kernel_end)
{
  int res;

  uart_send_string("Initializing memory system...\n");
  uart_send_string("RAM size: ");
  uart_send_string(uint_to_str(ram_size / (1024 * 1024)));
  uart_send_string(" MB\n");

  uart_send_string("Kernel: 0x");
  uart_send_string(uint_to_str(kernel_start));
  uart_send_string(" - 0x");
  uart_send_string(uint_to_str(kernel_end));
  uart_send_string("\n");

  // Step 1: Initialize basic kernel heap
  uart_send_string("Initializing kernel heap...\n");
  kheap_init(ram_size);
  uart_send_string("Heap initialized!\n");

  // Step 2: Initialize MMU
  uart_send_string("Initializing MMU...\n");
  res = kernel_mmu_init(ram_size);
  if (res < 0)
  {
    uart_send_string("Failed to initialize MMU\n");
    return res;
  }

  // Step 3: Create identity mappings for critical regions

  // Identity map the first 16MB of RAM for boot code and early kernel
  uart_send_string("Creating identity mappings for boot code and early kernel...\n");
  res = kernel_mmu_identity_map(0, 16 * 1024 * 1024, MAP_READ | MAP_WRITE | MAP_EXEC | MAP_CACHE_WB);
  if (res < 0)
  {
    uart_send_string("Failed to create identity mappings\n");
    return res;
  }

  // Identity map MMIO regions (in a real system, these would be hardware specific)
  // For now, just map UART
  uart_send_string("Creating identity mappings for MMIO regions...\n");
  res = kernel_mmu_identity_map(0x09000000, 0x1000, MAP_READ | MAP_WRITE | MAP_DEVICE);
  if (res < 0)
  {
    uart_send_string("Failed to map MMIO regions\n");
    return res;
  }

  // Add the new identity mapping here:
  uart_send_string("Identity mapping entire kernel area...\n");
  res = kernel_mmu_identity_map(kernel_start & ~(PAGE_SIZE-1), 
                            (kernel_end - kernel_start + PAGE_SIZE*2) & ~(PAGE_SIZE-1),
                            MAP_READ | MAP_WRITE | MAP_EXEC | MAP_CACHE_WB);
  if (res < 0)
  {
    uart_send_string("Failed to map kernel area\n");
    return res;
  }

  // Step 4: Initialize paging system
  uart_send_string("Initializing paging system...\n");
  res = paging_init(ram_size, kernel_start, kernel_end);
  if (res < 0)
  {
    uart_send_string("Failed to initialize paging system\n");
    return res;
  }

  // // Step 5: Enable MMU
  // uart_send_string("Enabling MMU...\n");
  // res = kernel_mmu_enable();
  // if (res < 0)
  // {
  //   uart_send_string("Failed to enable MMU\n");
  //   return res;
  // }

  // Step 6: Initialize AI memory subsystem
  size_t ai_pool_size = ram_size / AI_MEMORY_POOL_RATIO;
  uart_send_string("Initializing AI memory with ");
  uart_send_string(uint_to_str(ai_pool_size / (1024 * 1024)));
  uart_send_string(" MB pool...\n");

  res = ai_memory_init(ai_pool_size);
  if (res < 0)
  {
    uart_send_string("Failed to initialize AI memory\n");
    return res;
  }

  // Step 7: Register memory regions for tracking

  // Add RAM region
  add_memory_region(0, 0, ram_size, MEM_REGION_TYPE_RAM, "System RAM");

  // Add kernel region
  add_memory_region(kernel_start, kernel_start, kernel_end - kernel_start, MEM_REGION_TYPE_KERNEL, "Kernel");

  // Add UART region
  add_memory_region(0x09000000, 0x09000000, 0x1000, MEM_REGION_TYPE_MMIO, "UART");

  uart_send_string("Memory system initialization complete\n");

  return EOK;
}

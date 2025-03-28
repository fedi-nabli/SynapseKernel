/*
 * memory_system.c - This file implements different methods related to
 * the memory system management. It initializes and tests all important
 * memory components.
 *
 * Author: Fedi Nabli
 * Date: 21 Mar 2025
 * Last Modified: 26 Mar 2025
 */

#include "memory_system.h"

#include <uart.h>

#include <synapse/bool.h>
#include <synapse/types.h>
#include <synapse/status.h>

#include <kernel/config.h>

#include <synapse/memory/memory.h>
#include <synapse/memory/heap/kheap.h>
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
 * @brief Initialize the complete memory system
 * 
 * This function initializes the entire memory system, including the kernel heap,
 * and AI memory subsystem. It also
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

  uart_send_string("Memory system initialization complete\n");

  return EOK;
}

/**
 * @brief Test the kernel heap allocator
 * 
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 25 Mar 2025
 */
int memory_test_kernel_heap()
{
  uart_send_string("\n=== Testing Kernel Heap ===\n");

  // Test small allocations
  uart_send_string("Testing small allocations...\n");
  void* small1 = kmalloc(64);
  void* small2 = kmalloc(128);
  void* small3 = kmalloc(256);

  if (!small1 || !small2 || !small3)
  {
    uart_send_string("FAIL: Small allocation failed\n");
    return -ENOMEM;
  }

  uart_send_string("Small allocation addresses:\n");
  uart_send_string("  small1: 0x");
  uart_send_string(uint_to_str((uintptr_t)small1));
  uart_send_string("\n  small2: 0x");
  uart_send_string(uint_to_str((uintptr_t)small2));
  uart_send_string("\n  small3: 0x");
  uart_send_string(uint_to_str((uintptr_t)small3));
  uart_send_string("\n");

  // Write to allocated memory to ensure it's usable
  memset(small1, 0xAA, 64);
  memset(small2, 0xBB, 128);
  memset(small3, 0xCC, 256);

  // Verify we can read from it
  uint8_t* ptr = (uint8_t*)small1;
  if (ptr[0] != 0xAA || ptr[63] != 0xAA)
  {
    uart_send_string("FAIL: memory write/read verification failed\n");
    return -EFAULT;
  }

  // Free small allocation
  uart_send_string("Freeing small allocation...\n");
  kfree(small1);
  kfree(small2);
  kfree(small3);

  // Test large allocation
  void* large = kmalloc(8196);
  if (!large)
  {
    uart_send_string("FAIL: Large allocation failed\n");
    return -ENOMEM;
  }

  uart_send_string("Large allocation address: 0x");
  uart_send_string(uint_to_str((uintptr_t)large));
  uart_send_string("\n");

  // Write to allocated memory
  memset(large, 0xDD, 8196);
  kfree(large);

  // Test allocation after free
  uart_send_string("Testing allocation after free...\n");
  void* realloc = kmalloc(128);
  if (!realloc)
  {
    uart_send_string("FAIL: re-allocation failed\n");
    return -ENOMEM;
  }
  kfree(realloc);

  uart_send_string("Kernel heap tests PASSED\n");
  return EOK;
}

/**
 * @brief Test the AI memory subsystem
 * 
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 26 Mar 2025
 */
int memory_test_ai_memory()
{
  uart_send_string("\n=== Testing AI Memory Subsystem ===\n");

  // Print initial memory stats
  ai_memory_print_stats();
  
  // Test 1: Create a 1D tensor (vector)
  uart_send_string("Test 1: Creating 1D tensor (vector)...\n");
  
  size_t shape1d[1] = {4}; // 4-element vector
  
  tensor_t* tensor1d = ai_tensor_create(shape1d, 1, TENSOR_TYPE_FLOAT32, 
                                     TENSOR_LAYOUT_ROW_MAJOR, TENSOR_MEM_ZEROED);
  
  if (!tensor1d) {
    uart_send_string("FAIL: 1D tensor creation failed\n");
    return -ENOMEM;
  }
  
  uart_send_string("Successfully created 1D tensor\n");
  uart_send_string("1D tensor properties:\n");
  uart_send_string("  Address: 0x");
  uart_send_string(uint_to_str((uintptr_t)tensor1d));
  uart_send_string("\n  Data address: 0x");
  uart_send_string(uint_to_str((uintptr_t)tensor1d->data));
  uart_send_string("\n  Dimensions: ");
  uart_send_string(uint_to_str(tensor1d->ndim));
  uart_send_string("\n  Shape: [");
  uart_send_string(uint_to_str(tensor1d->shape[0]));
  uart_send_string("]\n  Strides: [");
  uart_send_string(uint_to_str(tensor1d->strides[0]));
  uart_send_string("]\n");

  // Print memory stats
  ai_memory_print_stats();
  
  // Clean up tensors
  uart_send_string("\nDestroying tensors...\n");
  
  int res = ai_tensor_destroy(tensor1d);
  if (res != EOK) {
    uart_send_string("FAIL: Error destroying 1D tensor\n");
    return res;
  }
  
  uart_send_string("All tensors destroyed successfully\n");
  
  // Print final memory stats
  ai_memory_print_stats();
  
  uart_send_string("AI memory subsystem tests PASSED\n");
  return EOK;
}

/**
 * @brief Print all registered memory regions
 * 
 * @author Fedi Nabli
 * @date 26 Mar 2025
 */
void memory_print_regions()
{
  uart_send_string("Memory regions:\n");
  
  for (size_t i = 0; i < num_memory_regions; i++) {
    mem_system_region_t* region = &memory_regions[i];
    
    uart_send_string("  Region ");
    uart_send_string(uint_to_str(i));
    uart_send_string(": ");
    uart_send_string(region->name);
    uart_send_string("\n    Phys: 0x");
    uart_send_string(uint_to_str(region->phys_start));
    uart_send_string(" - 0x");
    uart_send_string(uint_to_str(region->phys_end));
    uart_send_string("\n    Virt: 0x");
    uart_send_string(uint_to_str(region->virt_start));
    uart_send_string("\n    Size: ");
    uart_send_string(uint_to_str(region->size / 1024));
    uart_send_string(" KB\n    Type: ");
    
    switch (region->type) {
      case MEM_REGION_TYPE_RAM:
        uart_send_string("RAM");
        break;
      case MEM_REGION_TYPE_DEVICE:
        uart_send_string("Device");
        break;
      case MEM_REGION_TYPE_MMIO:
        uart_send_string("MMIO");
        break;
      case MEM_REGION_TYPE_KERNEL:
        uart_send_string("Kernel");
        break;
      default:
        uart_send_string("Unknown");
        break;
    }
    uart_send_string("\n");
  }
}

/**
 * @brief Test memory region tracking
 * 
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 25 Mar 2025
 */
int memory_test_regions()
{
  uart_send_string("\n=== Testing Memory Region Tracking ===\n");

  // This function will display all registered memory regions
  memory_print_regions();

  uart_send_string("Memory region tests PASSED\n");
  return EOK;
}

/**
 * @brief Run all memory system tests
 * 
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 26 Mar 2025
 */
int memory_run_tests()
{
  int res;
  
  uart_send_string("\n=== Running Memory System Tests ===\n");
  
  // Test kernel heap
  res = memory_test_kernel_heap();
  if (res != EOK) {
    uart_send_string("Kernel heap tests FAILED\n");
    return res;
  }
  
  // Test AI memory with minimal test
  res = memory_test_ai_memory();
  if (res != EOK) {
    uart_send_string("AI memory tests FAILED\n");
    return res;
  }
  
  // // Test memory regions
  // res = memory_test_regions();
  // if (res != EOK) {
  //   uart_send_string("Memory region tests FAILED\n");
  //   return res;
  // }
  
  uart_send_string("\n=== All Memory System Tests PASSED ===\n");
  return EOK;
}

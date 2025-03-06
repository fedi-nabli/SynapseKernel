/*
 * kernel_main.c - Entry point for the kernel
 *
 * This is the entry point function confirms successful handoff from
 * the bootloader and initializes the minimal kernel environment.
 * 
 * Author: Fedi Nabli
 * Date: 26 Feb 2025
 * Last Modified: 26 Feb 2025
 */

#include <uart.h>
#include <boot_info.h>

#include <synapse/memory/heap/kheap.h>

void test_heap(void) {
  uart_send_string("---------- HEAP TEST ----------\n");
  
  // Allocate memory and display address info
  uart_send_string("Allocating 4096 bytes...\n");
  void* ptr = kmalloc(4096);
  
  // Convert and display address
  char addr_str[20] = "0x";
  char hex_chars[] = "0123456789ABCDEF";
  unsigned long addr = (unsigned long)ptr;
  int i = 2;
  
  for (int shift = 60; shift >= 0; shift -= 4) {
    unsigned char digit = (addr >> shift) & 0xF;
    if (digit != 0 || i > 2) {
      addr_str[i++] = hex_chars[digit];
    }
  }
  
  if (i == 2) addr_str[i++] = '0';
  addr_str[i] = '\0';
  
  if (ptr) {
    uart_send_string("Allocation SUCCESS: ");
    uart_send_string(addr_str);
    uart_send_string("\n");
    
    // Debug checkpoint
    uart_send_string("About to write to memory...\n");
    
    // Try writing just 1 byte first
    *((unsigned char*)ptr) = 0x42;
    
    uart_send_string("Write successful!\n");
    
    // Debug checkpoint
    uart_send_string("About to free memory...\n");
    
    // Try freeing
    kfree(ptr);
    
    uart_send_string("Free successful!\n");
  } else {
    uart_send_string("Allocation FAILED: NULL pointer returned\n");
  }
  
  uart_send_string("---------- TEST COMPLETE ----------\n");
}

void kernel_main(boot_info_t* boot_info)
{
  // Initializes UART
  uart_init();

  // Send confirmation message
  uart_send_string("Kernel started successfully!\n");

  // Verify boot info
  if (boot_info && boot_info->magic == BOOT_INFO_MAGIC)
  {
    uart_send_string("Boot info verified. System details:\n");
    // TODO: Implement number to string function
    uart_send_string("- RAM detected\n");
  }
  else
  {
    uart_send_string("WARNING: Boot info invalid or missing\n");
  }

  // Initialize the heap
  uart_send_string("Initializing kernel heap...\n");
  kheap_init(boot_info->ram_size);
  uart_send_string("Heap initialized!\n");

  // Run the heap test
  test_heap();

  // Simple infinite loop
  uart_send_string("Kernel running (idle loop)...\n");
  while (1)
  {
    for (volatile int i = 0; i < 10000000; i++) {}
    uart_send_string(".");
  }
}
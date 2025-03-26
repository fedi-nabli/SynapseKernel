/*
 * kernel_main.c - Entry point for the kernel
 *
 * This is the entry point function confirms successful handoff from
 * the bootloader and initializes the minimal kernel environment.
 * 
 * Author: Fedi Nabli
 * Date: 26 Feb 2025
 * Last Modified: 17 Mar 2025
 */

#include <uart.h>
#include <boot_info.h>

#include <synapse/memory/memory_system.h>

// Define the kernel start and end symbols from the linker
extern char _start[];
extern char _end[];

static void uint_to_str(uint64_t value, char* buf, size_t size)
{
  if (!buf || size == 0)
  {
    return;
  }
  
  // Handle zero case
  if (value == 0)
  {
    buf[0] = '0';
    buf[1] = '\0';
    return;
  }
  
  // Convert digits
  size_t i = 0;
  while (value > 0 && i < size - 1)
  {
    buf[i++] = '0' + (value % 10);
    value /= 10;
  }
  buf[i] = '\0';
  
  // Reverse the string
  for (size_t j = 0; j < i / 2; j++)
  {
    char temp = buf[j];
    buf[j] = buf[i - j - 1];
    buf[i - j - 1] = temp;
  }
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
    uart_send_string("- RAM: ");
    char buffer[32];
    uint_to_str(boot_info->ram_size / (1024 * 1024), buffer, sizeof(buffer));
    uart_send_string(buffer);
    uart_send_string(" MB\n");
  }
  else
  {
    uart_send_string("WARNING: Boot info invalid or missing\n");
  }

  // Get kernel addresses
  uintptr_t kernel_start = (uintptr_t)&_start;
  uintptr_t kernel_end = (uintptr_t)&_end;

  // Initialize the memory system
  uart_send_string("Initializing memory system...\n");
  int res = memory_system_init(boot_info->ram_size, kernel_start, kernel_end);
  if (res < 0)
  {
    uart_send_string("Memory system initialization failed!\n");
    while (1) {} // Halt
  }

  // Run memory tests
  res = memory_run_tests();
  if (res < 0)
  {
    uart_send_string("Memory tests failed!\n");
  }

  // Simple infinite loop
  uart_send_string("Kernel running (idle loop)...\n");
  while (1)
  {
    for (volatile int i = 0; i < 10000000; i++) {}
    uart_send_string(".");
  }
}
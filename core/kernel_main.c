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

  // Simple infinite loop
  uart_send_string("Kernel running (idle loop)...\n");
  while (1)
  {
    for (volatile int i = 0; i < 10000000; i++) {}
    uart_send_string(".");
  }
}
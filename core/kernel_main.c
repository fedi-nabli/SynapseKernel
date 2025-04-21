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

#include <synapse/process/process.h>
#include <synapse/interrupts/syscall.h>
#include <synapse/memory/memory_system.h>

// Define the kernel start and end symbols from the linker
extern char _start[];
extern char _end[];

// Test process functions
void kernel_process_test();
void user_process_test();

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

  // Initialize process management subsystem
  uart_send_string("\n=== Testing Process Management ===\n");
  res = process_management_init();
  if (res < 0)
  {
    uart_send_string("Process management initialization failed!\n");
    while (1) {} // Halt
  }

  // Create a kernel process
  res = create_kernel_process(kernel_process_test, "kernel_test");
  if (res < 0)
  {
    uart_send_string("Failed to create kernel test process!\n");
    while (1) {} // Halt
  }

  // Create a user process
  res = create_user_process(user_process_test, "user_test");
  if (res < 0)
  {
    uart_send_string("Failed to create user test process!\n");
    while (1) {} // Halt
  }

  // Start process management
  uart_send_string("Starting process management.m..\n");
  res = process_management_start();
  
  // We should never reach here if process management is working correctly
  uart_send_string("ERROR: Process management returned unexpectedly!\n");
  while (1) {} // Halt

  // Simple infinite loop
  uart_send_string("Kernel running (idle loop)...\n");
  while (1)
  {
    for (volatile int i = 0; i < 10000000; i++) {}
    uart_send_string(".");
  }
}

// Kernel process test function
void kernel_process_test()
{
  uart_send_string("[KERNEL PROCESS] Started kernel process test\n");

  // Do some work
  for (int i = 0; i < 5; i++)
  {
    uart_send_string("[KERNEL PROCESS] Working...\n");

    // Simulate work
    for (volatile int j = 0; j < 1000000; j++) {}
  }

  void* mem = process_malloc(process_current(), 128);
  
  if (mem)
  {
    uart_send_string("[KERNEL PROCESS] Memory allocation successful\n");
    
    // Free memory
    process_free(process_current(), mem);
    uart_send_string("[KERNEL PROCESS] Memory freed\n");
  }
  else
  {
    uart_send_string("[KERNEL PROCESS] Memory allocation failed\n");
  }

  uart_send_string("[KERNEL PROCESS] Kernel process test finished\n");

  // Exit the process
  process_terminate(process_current()->id);
}

// User process test function
void user_process_test(void)
{
  uart_send_string("[USER PROCESS] Started user process test\n");
  
  struct process* me = process_current();
    
  // Do some work
  for (int i = 0; i < 5; i++)
  {
    uart_send_string("[USER PROCESS] Working...\n");
    
    // Simulate work
    for (volatile int j = 0; j < 1000000; j++) {}
  }
  
  uart_send_string("[USER PROCESS] User process test complete\n");
  
  // Exit the process
  process_terminate(me->id);
}

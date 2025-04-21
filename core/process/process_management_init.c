/*
 * process_management_init.c - Initialization of process management subsystem for AArch64
 *
 * Author: Fedi Nabli
 * Date: 9 Apr 2025
 * Last Modified: 9 Apr 2025
 */

#include <uart.h>

#include <synapse/status.h>

#include <synapse/task/task.h>
#include <synapse/timer/timer.h>
#include <synapse/process/process.h>
#include <synapse/interrupts/syscall.h>
#include <synapse/scheduler/scheduler.h>
#include <synapse/interrupts/interrupt.h>

/**
 * @brief Initialize process management subsystem
 * 
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 9 Apr 2025
 */
int process_management_init()
{
  int res;

  uart_send_string("Initializing process management subsystem...\n");

  // Initialize interrupt subsystem
  uart_send_string("Initializing interrupt subsystem...\n");
  res = interrupt_init();
  if (res < 0)
  {
    uart_send_string("Failed to initialize interrupt subsystem!\n");
    return res;
  }
  uart_send_string("Interrupt subsystem initialized.\n");

  // Initialize system call interface
  uart_send_string("Initializing system call interface...\n");
  res = syscall_init();
  if (res < 0)
  {
    uart_send_string("Failed to initialize system call interface!\n");
    return res;
  }
  uart_send_string("System call interface initialized.\n");

  // Initialize scheduler
  uart_send_string("Initializing scheduler...\n");
  res = scheduler_init();
  if (res < 0)
  {
    uart_send_string("Failed to initialize scheduler\n");
    return res;
  }
  uart_send_string("Scheduler initialized.\n");

  uart_send_string("Process management subsystem initialized successfully!\n");
  return EOK;
}

/**
 * @brief Create and start initial kernel process
 * 
 * @param entry_point Function pointer to process entry point
 * @param name Process name
 * @return int Process ID on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 9 Apr 2025
 */
int create_kernel_process(void (*entry_point)(void), const char* name)
{
  int res;
  struct process* process;

  // Create a dummy buffer for the process creation
  char dummy_buffer[8] = {0};
  
  // Create process with dummy buffer
  res = process_create(name, dummy_buffer, sizeof(dummy_buffer), &process);
  if (res < 0)
  {
    uart_send_string("Failed to create kernel process\n");
    return res;
  }

  process->task->registers.pc = (reg_t)entry_point;
  process->task->registers.elr_el1 = (reg_t)entry_point;

  // Set kernel mode (SPSR_EL1 = 0x5, EL1h)
  // This is for EL1 with SPSel=1 (using SP_EL1)
  process->task->registers.spsr_el1 = 0x305; // EL1h | SPsel=1 | FIQ masked

  // Mark task as ready
  process->task->state = TASK_STATE_READY;

  uart_send_string("Created kernel process: ");
  uart_send_string(name);
  uart_send_string("\n");

  return process->id;
}

/**
 * @brief Create and start initial user process
 * 
 * @param entry_point Function pointer to process entry point
 * @param name Process name
 * @return int Process ID on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 9 Apr 2025
 */
int create_user_process(void (*entry_point)(void), const char* name)
{
  int res;
  struct process* process;

  // Create a dummy buffer for the process creation
  char dummy_buffer[8] = {0};
  
  // Create process with dummy buffer
  res = process_create(name, dummy_buffer, sizeof(dummy_buffer), &process);
  if (res < 0)
  {
    uart_send_string("Failed to create user process\n");
    return res;
  }

  process->task->registers.pc = (reg_t)entry_point;
  process->task->registers.elr_el1 = (reg_t)entry_point;

  // Set user mode (SPSR_EL1 = 0x0, EL0t)
  // This is for EL0 with SPSel=0 (using SP_EL0)
  process->task->registers.spsr_el1 = 0x305; // EL0t with interrupts enabled

  // Mark task as ready
  process->task->state = TASK_STATE_READY;

  uart_send_string("Created user process: ");
  uart_send_string(name);
  uart_send_string("\n");

  return process->id;
}

/**
 * @brief Start the process management subsystem and run initial task
 * 
 * @return int Never returns on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 9 Apr 2025
 */
int process_management_start()
{
  int res;

  uart_send_string("Starting process management subsystem...\n");

  // Start scheduler
  res = scheduler_start();
  if (res < 0)
  {
    uart_send_string("Failed to start scheduler\n");
    return res;
  }

  // If we got here, something went wrong (scheduler should not return)
  uart_send_string("ERROR: Scheduler returned unexpectedly\n");
  return -EFAULT;
}

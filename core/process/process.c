/*
 * process.c - Process management implementation for AArch64-based kernel
 *
 * Author: Fedi Nabli
 * Date: 2 Apr 2025
 * Last Modified: 8 Apr 2025
 */

#include "process.h"

#include <uart.h>

#include <kernel/config.h>

#include <synapse/bool.h>
#include <synapse/types.h>
#include <synapse/status.h>

#include <synapse/task/task.h>
#include <synapse/string/string.h>
#include <synapse/memory/memory.h>
#include <synapse/memory/heap/kheap.h>

int process_switch(pid_t id);

// Process table
static struct process* process_table[SYNAPSE_MAX_PROCESSES] = {0};
static pid_t current_process = 0;

// Helper function to convert uint64_t to string
static void uint64_to_str(uint64_t value, char* buffer, size_t buffer_size) {
  if (buffer_size < 3) return; // Need at least space for "0x" and null terminator
  
  buffer[0] = '0';
  buffer[1] = 'x';
  
  size_t pos = 2;
  bool significant = false;
  
  for (ssize_t i = 60; i >= 0 && pos < buffer_size - 1; i -= 4) {
    int digit = (value >> i) & 0xF;
    if (digit != 0 || significant || i == 0) {
      buffer[pos++] = digit < 10 ? '0' + digit : 'A' + (digit - 10);
      significant = true;
    }
  }
  
  buffer[pos] = '\0';
}

// Safe exit handler for tasks that return unexpectedly
void process_return_handler(void)
{
  uart_send_string("Error: Process returned unexpectedly. Halting.\n");
  while (1) { /* Halt */ }
}

/**
 * @brief Allocate a new process ID
 * 
 * @return int Allocated process ID, or negative error code
 * 
 * @author Fedi Nabli
 * @date 2 Apr 2025
 */
static int process_allocate_slot()
{
  for (int i = 0; i < SYNAPSE_MAX_PROCESSES; i++)
  {
    if (process_table[i] == NULL)
    {
      return i;
    }
  }

  // Maximum processes reached
  return -EPMAX;
}

/**
 * @brief Initialize a process structure
 * 
 * @param process Process structure to initialize
 * @param id Pross ID to assign
 * @param name Process name
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 3 Apr 2025
 */
static int process_init(struct process* process, pid_t id, const char* name)
{
  if (!process || !name)
  {
    return -EINVARG;
  }

  // Initialize the process structure
  memset(process, 0, sizeof(struct process));
  process->id = id;
  strncpy(process->name, name, SYNAPSE_MAX_PROCESS_NAME);

  return EOK;
}

/**
 * @brief Load binary data into process
 * 
 * @param process Process structure
 * @param program_data Pointer to program data
 * @param size Size of program hostory
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 7 Apr 2025
 */
static int process_load_binary(struct process* process, void* program_data, size_t size)
{
  if (!process || !program_data || size == 0)
  {
    return -EINVARG;
  }

  // Allocate memory fpr program code
  void* process_program_data = process_malloc(process, size);
  if (!process_program_data)
  {
    return -ENOMEM;
  }

  // Copy program data
  memcpy(process_program_data, program_data, size);

  // ensure freshly-copied code is visible to the CPU
  process_memory_flush_icache(process_program_data, size);

  // Set up process program data
  process->ptr = process_program_data;
  process->size = size;

  return EOK;
}

/**
 * @brief Allocate and setup stack for a process
 * 
 * @param process Process structure
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 7 Apr 2025
 */
static int process_allocate_stack(struct process* process)
{
  void* stack = process_malloc(process, SYNAPSE_PROCESS_STACK_SIZE);
  if (!stack)
  {
    return -ENOMEM;
  }

  // Zero-initialize stack
  memset(stack, 0, SYNAPSE_PROCESS_STACK_SIZE);

  process->stack = stack;
  return EOK;
}

/**
 * @brief Create main task for process
 * 
 * @param process Process structure
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 7 Apr 2025
 */
static int process_create_task(struct process* process)
{
  uart_send_string("process_create_task: Creating task\n");
  
  struct task* task = task_new(TASK_PRIORITY_NORMAL);
  if (!task)
  {
    uart_send_string("process_create_task: Failed to allocate task\n");
    return -ENOMEM;
  }

  // Initialize linkage between process and task
  task->process = process;
  process->task = task;

  uart_send_string("Setting up task registers\n");

  // Set program counter and exception link register
  task->registers.pc = (reg_t)process->ptr;
  task->registers.elr_el1 = (reg_t)process->ptr;

  // Compute raw stack top and align to 16 bytes
  uint64_t stack_base = (uint64_t)process->stack;
  uint64_t stack_size = SYNAPSE_PROCESS_STACK_SIZE;
  uint64_t sp_value = (stack_base + stack_size) & ~15ULL;
  task->registers.sp = sp_value;

  // Initialize link register (X30) to safe exit handler
  task->registers.x30 = (reg_t)process_return_handler;
  uart_send_string("Task link register (X30) set to safe exit handler\n");

  // Set SPSR_EL1 based on process type
  if (process->id == 0) {
    // Kernel task (EL1h)
    task->registers.spsr_el1 = 0x305;
    uart_send_string("SPSR_EL1 set for kernel mode (0x305)\n");
  } else {
    // User task (EL0t)
    task->registers.spsr_el1 = 0x305;
    uart_send_string("SPSR_EL1 set for user mode (0x305)\n");
  }

  // Mark task as ready
  task->state = TASK_STATE_READY;
  uart_send_string("Task state set to READY\n");

  return EOK;
}

/**
 * @brief Get the currently running process
 * 
 * @return struct process* Pointer to current process, NULL if none is running
 * 
 * @author Fedi Nabli
 * @date 2 Apr 2025
 */
struct process* process_current()
{
  if (current_process >= SYNAPSE_MAX_PROCESSES)
  {
    return NULL;
  }

  return process_table[current_process];
}

/**
 * @brief Get a process by ID
 * 
 * @param id Process ID to look up
 * @return struct process* Pointer to process, NULL if not found
 * 
 * @author Fedi Nabli
 * @date 2 Apr 2025
 */
struct process* process_get(pid_t id)
{
  if (id >= SYNAPSE_MAX_PROCESSES)
  {
    return NULL;
  }

  return process_table[id];
}

/**
 * @brief Create and load a process from memory
 * 
 * @param name Process name
 * @param program_data Pointer to program binary
 * @param size Size of program binary
 * @param process_out Output parameter for created process
 * @return int Process ID on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 7 Apr 2025
 */
int process_create(const char* name, void* program_data, size_t size, struct process** process_out)
{
  int res;

  uart_send_string("process_create: Creating process '");
  uart_send_string(name);
  uart_send_string("'\n");

  if (!name || !program_data || size == 0)
  {
    uart_send_string("process_create: Invalid arguments\n");
    return -EINVARG;
  }

  // Allocate process table slot
  int slot = process_allocate_slot();
  if (slot < 0)
  {
    uart_send_string("process_create: Failed to allocate process slot\n");
    return slot;
  }

  // Allocate process structure
  struct process* process = kmalloc(sizeof(struct process));
  if (!process)
  {
    uart_send_string("process_create: Failed to allocate process structure\n");
    return -ENOMEM;
  }

  // Initialize process
  res = process_init(process, slot, name);
  if (res < 0)
  {
    uart_send_string("process_create: Failed to initialize process\n");
    kfree(process);
    return res;
  }

  // Allocate stack
  uart_send_string("process_create: Allocating stack\n");
  res = process_allocate_stack(process);
  if (res < 0)
  {
    uart_send_string("process_create: Failed to allocate stack\n");
    kfree(process);
    return res;
  }

  // Log stack address
  char buf[20];
  uint64_to_str((uint64_t)process->stack, buf, sizeof(buf));
  uart_send_string("Stack address: ");
  uart_send_string(buf);
  uart_send_string("\n");

  // Load binary
  uart_send_string("process_create: Loading binary\n");
  res = process_load_binary(process, program_data, size);
  if (res < 0)
  {
    uart_send_string("process_create: Failed to load binary\n");
    process_free(process, process->stack);
    kfree(process);
    return res;
  }

  // Log program address
  uint64_to_str((uint64_t)process->ptr, buf, sizeof(buf));
  uart_send_string("Program address: ");
  uart_send_string(buf);
  uart_send_string("\n");

  // Create task
  uart_send_string("process_create: Creating task\n");
  res = process_create_task(process);
  if (res < 0)
  {
    uart_send_string("process_create: Failed to create task\n");
    process_free(process, process->stack);
    process_free(process, process->ptr);
    kfree(process);
    return res;
  }

  // Store in process table
  uart_send_string("process_create: Storing in process table\n");
  process_table[slot] = process;

  // Set output parameter
  if (process_out)
  {
    *process_out = process;
  }

  uart_send_string("process_create: Process created successfully\n");
  return slot;
}

/**
 * @brief Create a process and immediatly switch to it
 * 
 * @param name Process name
 * @param program_data Pointer to program binary
 * @param size Size of program binary
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 7 Apr 2025
 */
int process_create_switch(const char* name, void* program_data, size_t size)
{
  struct process* process;
  int process_id = process_create(name, program_data, size, &process);

  if (process_id < 0)
  {
    return process_id;
  }

  // Switch to the new process
  process_switch(process_id);

  return EOK;
}

/**
 * @brief Create a process in a specific slot
 * 
 * @param name Process name
 * @param program_data Pointer to program binary
 * @param size Size of program binary
 * @param slot Process slot/ID to use
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 7 Apr 2025
 */
int process_create_for_slot(const char* name, void* program_data, size_t size, int slot)
{
  if (slot < 0 || slot >= SYNAPSE_MAX_PROCESSES)
  {
    return -EINVARG;
  }

  if (process_table[slot] != NULL)
  {
    // Slot already in use
    return -EINUSE;
  }

  struct process* process;
  int res = process_create(name, program_data, size, &process);

  if (res >= 0 && res != slot)
  {
    // Update process ID and table entry
    process->id = slot;
    process_table[res] = NULL; // Clear original slot
    process_table[slot] = process;
    res = EOK;
  }

  return res;
}

/**
 * @brief Switch to different process by ID
 * 
 * @param id Process ID to switch to
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 7 Apr 2025
 */
int process_switch(pid_t id)
{
  if (id >= SYNAPSE_MAX_PROCESSES || process_table[id] == NULL)
  {
    return -EINVARG;
  }

  char buf[20];
  uint64_to_str(id, buf, sizeof(buf));
  uart_send_string("process_switch: current_process = ");
  uart_send_string(buf);
  uart_send_string("\n");

  // Save current task state
  if (process_table[current_process] != NULL)
  {
    task_current_save_state();
  }

  // Switch to new process
  current_process = id;

  // Switch to main task of new process
  task_switch(process_table[id]->task);

  return EOK;
}

/**
 * @brief Terminate process and free resources
 * 
 * @param id Process ID to terminate
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 7 Apr 2025
 */
int process_terminate(pid_t id)
{
  if (id >= SYNAPSE_MAX_PROCESSES || process_table[id] == NULL)
  {
    return -EINVARG;
  }

  struct process* process = process_table[id];

  // Free allocations
  for (size_t i = 0; i < SYNAPSE_MAX_PROCESSES_ALLOCATIONS; i++)
  {
    if (process->allocations[i].ptr != NULL)
    {
      kfree(process->allocations[i].ptr);
      process->allocations[i].size = 0;
    }
  }

  // Free arguments
  if (process->arguments.argv != NULL)
  {
    for (int i = 0; i < process->arguments.argc; i++)
    {
      if (process->arguments.argv[i] != NULL)
      {
        kfree(process->arguments.argv[i]);
      }
    }

    kfree(process->arguments.argv);
  }

  // Free task
  task_free(process->task);

  // Clear table entry
  process_table[id] = NULL;

  // Free process structure
  kfree(process);

  // Schedule next task if we terminated the current process
  if (id == current_process)
  {
    current_process = SYNAPSE_MAX_PROCESSES; // Invalid ID to force finding new processes
    // task_schedule();
  }
  
  return EOK;
}

/**
 * @brief Get process arguments
 * 
 * @param id Process ID
 * @param argc Output for argument count
 * @param argv Output for argument vector
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 8 Apr 2025
 */
int process_get_arguments(pid_t id, int* argc, char*** argv)
{
  if (id >= SYNAPSE_MAX_PROCESSES || process_table[id] == NULL)
  {
    return -EINVARG;
  }

  if (argc != NULL)
  {
    *argc = process_table[id]->arguments.argc;
  }

  if (argv != NULL)
  {
    *argv = process_table[id]->arguments.argv;
  }

  return EOK;
}

/**
 * @brief Set process arguments
 * 
 * @param process Process to set arguments for
 * @param argc Argument count
 * @param argv Argument vector
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 8 Apr 2025
 */
int process_set_arguments(struct process* process, int argc, char** argv)
{
  if (!process || argc < 0)
  {
    return -EINVARG;
  }

  // Free existing arguments
  if (process->arguments.argv != NULL)
  {
    for (int i = 0; i < process->arguments.argc; i++)
    {
      if (process->arguments.argv[i] != NULL)
      {
        kfree(process->arguments.argv[i]);
      }
    }
    kfree(process->arguments.argv);
  }

  // No arguments to set
  if (argc == 0 || argv == NULL)
  {
    process->arguments.argc = 0;
    process->arguments.argv = NULL;
    return EOK;
  }

  // Allocate argument vector
  process->arguments.argv = kmalloc(sizeof(char*) * argc);
  if (!process->arguments.argv)
  {
    return -ENOMEM;
  }

  // Copy arguments
  for (int i = 0; i < argc; i++)
  {
    if (argv[i] != NULL)
    {
      size_t len = strlen(argv[i]) + 1;
      process->arguments.argv[i] = kmalloc(len);

      if (!process->arguments.argv[i])
      {
        // Free already allocated arguments
        for (int j = 0; j < i; j++)
        {
          kfree(process->arguments.argv[i]);
        }
        kfree(process->arguments.argv);
        process->arguments.argv = NULL;

        return -ENOMEM;
      }

      strcpy(process->arguments.argv[i], argv[i]);
    }
    else
    {
      process->arguments.argv[i] = NULL;
    }
  }

  process->arguments.argc = argc;

  return EOK;
}

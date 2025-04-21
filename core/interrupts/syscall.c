/*
 * syscall.c - System call interface for AArch64
 *
 * Author: Fedi Nabli
 * Date: 8 Apr 2025
 * Last Modified: 9 Apr 2025
 */

#include "syscall.h"

#include <uart.h>

#include <kernel/config.h>

#include <synapse/status.h>

#include <synapse/task/task.h>
#include <synapse/memory/memory.h>
#include <synapse/interrupts/svc.h>
#include <synapse/process/process.h>
#include <synapse/interrupts/interrupt.h>

// System call handler 
typedef int (*SYSCALL_HANDLER)(long arg1, long arg2, long arg3, long arg4);

// System call table
static SYSCALL_HANDLER syscall_table[SYSCALL_MAX] = {0};

/**
 * @brief Handler process exit system call
 * 
 * @param exit_code Process exit code
 * @param arg2 Not used
 * @param arg3 Not used
 * @param arg4 Not used
 * @return int Never returns to caller
 * 
 * @author Fedi Nabli
 * @date 8 Apr 2025
 */
static int syscall_process_exit(long exit_code, long arg2, long arg3, long arg4)
{
  struct process* current = process_current();
  if (current != NULL)
  {
    process_terminate(current->id);
  }

  // Schedule next task
  task_schedule();

  // Should never reach here
  return -ESYSCALL;
}

/**
 * @brief Handle memory allocation system call
 * 
 * @param size Size to allocate in bytes
 * @param arg2 Not used
 * @param arg3 Not used
 * @param arg4 Not used
 * @return int Pointer to allocated memory, or 0 on failure
 * 
 * @author Fedi Nabli
 * @date 8 Apr 2025
 */
static int syscall_process_malloc(long size, long arg2, long arg3, long arg4)
{
  struct process* current = process_current();
  if (current == NULL || size <= 0)
  {
    return 0;
  }

  void* ptr = process_malloc(current, size);
  return (int)(uint64_t)ptr;
}

/**
 * @brief Handler memory free system call
 * 
 * @param ptr Pointer to memory to free
 * @param arg2 Not used
 * @param arg3 Not used
 * @param arg4 Not used
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 8 Apr 2025
 */
static int syscall_process_free(long ptr, long arg2, long arg3, long arg4)
{
  struct process* current = process_current();
  if (current == NULL || ptr == 0)
  {
    return -EINVARG;
  }

  return process_free(current, (void*)(uint64_t)ptr);
}

/**
 * @brief Handle process arguments syscall
 * 
 * @param argc_ptr Pointer to store argument count
 * @param argv_ptr Pointer to store arguent vector
 * @param arg3 Not used
 * @param arg4 Not used
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 8 Apr 2025
 */
static int syscall_process_get_args(long argc_ptr, long argv_ptr, long arg3, long arg4)
{
  struct process* current = process_current();
  if (current == NULL)
  {
    return -EINVARG;
  }

  // Copy argc value if pointer provided
  if (argc_ptr != 0)
  {
    *((int*)(uint64_t)argc_ptr) = current->arguments.argc;
  }

  // Copy argv pointer if pointer provided
  if (argv_ptr != 0)
  {
    *((char***)(uint64_t)argv_ptr) = current->arguments.argv;
  }

  return EOK;
}

/**
 * @brief Handle print character syscall
 * 
 * @param character Character to print
 * @param arg2 Not used
 * @param arg3 Not used
 * @param arg4 Not used
 * @return int EOK on success, negative erro code on failure
 * 
 * @author Fedi Nabli
 * @date 8 Apr 2025
 */
static int syscall_internal_print_char(long character, long arg2, long arg3, long arg4)
{
  uart_send_char(character);
  return EOK;
}

/**
 * @brief Handle print string syscall
 * 
 * @param str Pointer to string to print
 * @param arg2 Not used
 * @param arg3 Not used
 * @param arg4 Not used
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 8 Apr 2025
 */
static int syscall_internal_print_string(long str, long arg2, long arg3, long arg4)
{
  if (str == 0)
  {
    return -EINVARG;
  }

  uart_send_string((const char*)str);
  return EOK;
}

/**
 * @brief Initialize system call interface
 * 
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 8 Apr 2025
 */
int syscall_init()
{
  // Register system call handlers
  syscall_table[SYSCALL_PROCESS_EXIT] = syscall_process_exit;
  syscall_table[SYSCALL_PROCESS_MALLOC] = syscall_process_malloc;
  syscall_table[SYSCALL_PROCESS_FREE] = syscall_process_free;
  syscall_table[SYSCALL_PROCESS_GET_ARGS] = syscall_process_get_args;
  syscall_table[SYSCALL_PRINT_CHAR] = syscall_internal_print_char;
  syscall_table[SYSCALL_PRINT_STRING] = syscall_internal_print_string;

  // SVC handler is setup in vector.S
  return svc_init(syscall_handler);
}

/**
 * @brief Main system call handler
 * 
 * @param syscall_num System call number
 * @param arg1 First argument
 * @param arg2 Second argument
 * @param arg3 Third argument
 * @param arg4 Forth argument
 * @return int System call result
 * 
 * @author Fedi Nabli
 * @date 8 Apr 2025
 */
int syscall_handler(int syscall_num, long arg1, long arg2, long arg3, long arg4)
{
  // Validate system call number
  if (syscall_num < 0 || syscall_num >= SYSCALL_MAX || syscall_table[syscall_num] == NULL)
  {
    return -EINVSYSCALL;
  }

  // Dispatch to handler
  return syscall_table[syscall_num](arg1, arg2, arg3, arg4);
}

/**
 * @brief Generic system call wraper
 * 
 * @param syscall_num System call number
 * @param arg1 First argument
 * @param arg2 Second argument
 * @param arg3 Third argument
 * @param arg4 Forth argument
 * @return int System call result
 * 
 * @author Fedi Nabli
 * @date 8 Apr 2025
 */
int syscall(int syscall_num, long arg1, long arg2, long arg3, long arg4)
{
  int result;

  // AArch64 inline assembly for SVC instruction
  __asm__ volatile(
    "mov x0, %1\n"
    "mov x1, %2\n"
    "mov x2, %3\n"
    "mov x3, %4\n"
    "mov x4, %5\n"
    "svc #0\n"
    "mov %0, x0\n"
    : "=r" (result)
    : "r" (syscall_num), "r" (arg1), "r" (arg2), "r" (arg3), "r" (arg4)
    : "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7"
  );

  return result;
}

/**
 * @brief Process exit system call wrapper
 * 
 * @param exit_code Exit code
 * 
 * @author Fedi Nabli
 * @date 9 Apr 2025
 */
void syscall_exit(int exit_code)
{
  syscall(SYSCALL_PROCESS_EXIT, exit_code, 0, 0, 0);

  // Should never return, but if it does, loop forever
  while (1) {}
}

/**
 * @brief Process memory allocation system call wrapper
 * 
 * @param size Size to allocate
 * @return void* Allocated memory, NULL on failure
 * 
 * @author Fedi Nabli
 * @date 9 Apr 2025
 */
void* syscall_malloc(size_t size)
{
  return (void*)(uint64_t)syscall(SYSCALL_PROCESS_MALLOC, size, 0, 0, 0);
}

/**
 * @brief Process memory free system call wrapper
 * 
 * @param ptr Pointer to free
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 9 Apr 2025
 */
int syscall_free(void* ptr)
{
  return syscall(SYSCALL_PROCESS_FREE, (uint64_t)ptr, 0, 0, 0);
}

/**
 * @brief Get process arguments system call wrapper
 * 
 * @param argc Pointer to store argument count
 * @param argv Pointer to store argument vector
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 9 Apr 2025
 */
int syscall_get_args(int* argc, char*** argv)
{
  return syscall(SYSCALL_PROCESS_GET_ARGS, (uint64_t)argc, (uint64_t)argv, 0, 0);
}

/**
 * @brief Print character system call wrapper
 * 
 * @param c Character to print
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 9 Apr 2025
 */
int syscall_print_char(char c)
{
  return syscall(SYSCALL_PRINT_CHAR, c, 0, 0, 0);
}

/**
 * @brief Print string system call wrapper
 * 
 * @param str String to print
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 9 Apr 2025
 */
int syscall_print_string(const char* str)
{
  return syscall(SYSCALL_PRINT_STRING, (uint64_t)str, 0, 0, 0);
}

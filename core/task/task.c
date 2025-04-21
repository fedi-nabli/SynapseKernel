/*
 * task.c - Task management implementation for AArch64
 *
 * Author: Fedi Nabli
 * Date: 31 Mar 2025
 * Last Modified: 31 Mar 2025
 */

#include "task.h"

#include <uart.h>

#include <kernel/config.h>

#include <synapse/bool.h>
#include <synapse/types.h>
#include <synapse/status.h>

#include <synapse/memory/memory.h>
#include <synapse/memory/heap/kheap.h>
#include <synapse/interrupts/interrupt.h>

// Task list
static struct task* task_list_head = NULL;
struct task* current_task = NULL; // Defined as global for assembly access
static tid_t next_task_id = 0;

/**
 * @brief Convert number to hex string (without stdlib)
 * 
 * @param value The value to convert
 * @param buffer Buffer to store the string
 * @param buffer_size Size of the buffer
 */
void uint64_to_hex(uint64_t value, char* buffer, size_t buffer_size) {
  if (buffer_size < 3) return; // Need at least "0x" and null
  
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

/**
 * @brief Get current running task
 * 
 * @return struct task* Current task, NULL if none is running
 * 
 * @author Fedi Nabli
 * @date 31 Mar 2025
 */
struct task* task_current()
{
  return current_task;
}

/**
 * @brief Create a new task
 * 
 * @return struct task* New task, NULL on failure
 * 
 * @author Fedi Nabli
 * @date 31 Mar 2025
 */
struct task* task_new(uint8_t task_priority)
{
  if (task_priority != TASK_PRIORITY_HIGH && task_priority != TASK_PRIORITY_LOW && task_priority != TASK_PRIORITY_NORMAL)
  {
    return NULL;
  }

  struct task* task = kmalloc(sizeof(struct task));
  if (!task)
  {
    return NULL;
  }

  // Initialize task
  memset(task, 0, sizeof(struct task));
  task->id = next_task_id++;
  task->state = TASK_STATE_NEW;
  task->priority = TASK_PRIORITY_NORMAL;
  if (task_priority)
  {
    task->priority = task_priority;
  }

  // Add to task list
  if (task_list_head == NULL)
  {
    // First task in list
    task_list_head = task;
    task->next = task;
    task->prev = task;
    goto out;
  }

  // Add to end of circular list
  task->next = task_list_head;
  task->prev = task_list_head->prev;
  task_list_head->prev->next = task;
  task_list_head->prev = task;

out:
  return task;
}

/**
 * @brief Free task and remove from list
 * 
 * @param task Task to free
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 31 Mar 2025
 */
int task_free(struct task* task)
{
  if (!task)
  {
    return -EINVARG;
  }

  // Only task in list
  if (task->next == task && task->prev == task)
  {
    // Clear head if this is the only task
    if (task_list_head == task)
    {
      task_list_head = NULL;
    }
  }
  else
  {
    // Update head if needed
    if (task_list_head == task)
    {
      task_list_head = task->next;
    }

    // Update circular list links
    task->prev->next = task->next;
    task->next->prev = task->prev;
  }

  // Handle current task
  if (current_task == task)
  {
    current_task = NULL;
  }

  // Free task structure
  kfree(task);
  
  return EOK;
}

/**
 * @brief Save task registers from interrupt frame
 * 
 * @param task Task to save state for
 * @param int_frame Interrupt frame with register values
 * @return int EOK on success, negative value on failure
 * 
 * @author Fedi Nabli
 * @date 31 Mar 2025
 */
int task_save_state(struct task* task, struct interrupt_frame* int_frame)
{
  if (!task || !int_frame)
  {
    return -EINVARG;
  }

  // Copy registers from interrupt frame
  task->registers.x0 = int_frame->x0;
  task->registers.x1 = int_frame->x1;
  task->registers.x2 = int_frame->x2;
  task->registers.x3 = int_frame->x3;
  task->registers.x4 = int_frame->x4;
  task->registers.x5 = int_frame->x5;
  task->registers.x6 = int_frame->x6;
  task->registers.x7 = int_frame->x7;
  task->registers.x8 = int_frame->x8;
  task->registers.x9 = int_frame->x9;
  task->registers.x10 = int_frame->x10;
  task->registers.x11 = int_frame->x11;
  task->registers.x12 = int_frame->x12;
  task->registers.x13 = int_frame->x13;
  task->registers.x14 = int_frame->x14;
  task->registers.x15 = int_frame->x15;
  task->registers.x16 = int_frame->x16;
  task->registers.x17 = int_frame->x17;
  task->registers.x18 = int_frame->x18;
  task->registers.x19 = int_frame->x19;
  task->registers.x20 = int_frame->x20;
  task->registers.x21 = int_frame->x21;
  task->registers.x22 = int_frame->x22;
  task->registers.x23 = int_frame->x23;
  task->registers.x24 = int_frame->x24;
  task->registers.x25 = int_frame->x25;
  task->registers.x26 = int_frame->x26;
  task->registers.x27 = int_frame->x27;
  task->registers.x28 = int_frame->x28;
  task->registers.x29 = int_frame->x29;
  task->registers.x30 = int_frame->x30;
  task->registers.sp = int_frame->sp;
  task->registers.pc = int_frame->elr_el1;
  task->registers.spsr_el1 = int_frame->spsr_el1;
  task->registers.elr_el1 = int_frame->elr_el1;

  return EOK;
}

/**
 * @brief Save current task state
 * 
 * @return int EOK on success, negative error on failure
 * 
 * @author Fedi Nabli
 * @date 31 Mar 2025
 */
int task_current_save_state()
{
  if (!current_task)
  {
    return -EINVARG;
  }

  // Call assembly functions to save state
  task_save_context();

  return EOK;
}

/**
 * @brief Switch to a different task
 * 
 * @param task Task to switch to
 * @return int negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 31 Mar 2025
 */
int task_switch(struct task* task)
{
  if (!task)
  {
    uart_send_string("task_switch: NULL task pointer\n");
    return -EINVARG;
  }
  
  uart_send_string("task_switch: found task...\n");
  
  // CRITICAL: Verify task has valid states before switching
  if (task->registers.sp == 0)
  {
    uart_send_string("task_switch: ERROR - task has NULL stack pointer\n");
    return -EFAULT;
  }
  
  if (task->registers.pc == 0)
  {
    uart_send_string("task_switch: ERROR - task has NULL program counter\n");
    return -EFAULT;
  }
  
  // Extra debug prints using your existing utility function
  char buf[32];
  uart_send_string("task_switch: task SP=");
  uint64_to_hex(task->registers.sp, buf, sizeof(buf));
  uart_send_string(buf);
  uart_send_string(" PC=");
  uint64_to_hex(task->registers.pc, buf, sizeof(buf));
  uart_send_string(buf);
  uart_send_string("\n");

  // CRITICAL: Explicitly set the current_task global before calling assembly
  current_task = task;
  
  // Set task state to running
  task->state = TASK_STATE_RUNNING;

  uart_send_string("task_switch: task now running...\n");

  // Switch context - will update current_task in assembly
  task_restore_context(task);

  uart_send_string("task_switch: ERROR - task_restore_context returned!\n");

  // Should never reach here - task_restore_context does not return if successful
  return -EFAULT;
}

/**
 * @brief Schedule next task to run
 * 
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 31 Mar 2025
 */
int task_schedule()
{
  if (task_list_head == NULL)
  {
    // No taslk to schedule
    return -ENOTASK;
  }

  // Simple round robin scheduling
  struct task* next = NULL;

  if (current_task == NULL)
  {
    // No current task, start with head
    next = task_list_head;
  }
  else
  {
    // Start from task after current one
    next = current_task->next;

    // Find ready task
    struct task* start = next;
    do
    {
      if (next->state == TASK_STATE_READY)
      {
        // Found ready task
        break;
      }
      next = next->next;

      // Avoid infinite loop by checking if we're gone full circle
      if (next == start)
      {
        // If we've checked all tasks and none are ready
        if (current_task->state == TASK_STATE_RUNNING)
        {
          // Continue with current task
          return EOK;
        }

        // No suitable task found
        return -ENOTASK;
      }
    } while (1);
  }

  // Switch to selected task
  return task_switch(next);
}

/**
 * @brief Run the first task in the system
 * 
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 31 Mar 2025
 */
int task_run_first_ever_task()
{
  if (task_list_head == NULL)
  {
    return -ENOTASK;
  }
  uart_send_string("task_run_first_ever_task: found task...\n");

  // Find first ready task
  struct task* task = task_list_head;
  struct task* start = task;

  do
  {
    if (task->state == TASK_STATE_READY)
    {
      uart_send_string("task_run_first_ever_task: found ready task...\n");
      // Switch to first ready task
      return task_switch(task);
    }
    task = task->next;

    // If we're gone through all tasks and none are ready
    if (task == start)
    {
      return -ENOTASK;
    }
  } while (1);
}

/**
 * @brief Return from task to the kernel
 * 
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 31 Mar 2025
 */
int task_return()
{
  if (!current_task)
  {
    return -EINVARG;
  }

  // Set task as finished
  current_task->state = TASK_STATE_FINISHED;

  // Schedule next task
  return task_schedule();
}

/**
 * @brief Block current task
 * 
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 31 Mar 2025
 */
int task_block()
{
  if (!current_task)
  {
    return -EINVARG;
  }

  // Set current task as blocked
  current_task->state = TASK_STATE_BLOCKED;

  // Schedule next task
  return task_schedule();
}

/**
 * @brief Unblock a task
 * 
 * @param task Task to unblock
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 31 Mar 2025
 */
int task_unblock(struct task* task)
{
  if (!task)
  {
    return -EINVARG;
  }

  // Only unblock if task is blocked
  if (task->state == TASK_STATE_BLOCKED)
  {
    task->state = TASK_STATE_READY;
  }

  return EOK;
}

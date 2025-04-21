/*
 * scheduler.c - Task scheduler for AArch64-based kernel
 *
 * Author: Fedi Nabli
 * Date: 9 Apr 2025
 * Last Modified: 9 Apr 2025
 */

#include "scheduler.h"

#include <uart.h>

#include <kernel/config.h>

#include <synapse/bool.h>
#include <synapse/types.h>
#include <synapse/status.h>

#include <synapse/task/task.h>
#include <synapse/timer/timer.h>
#include <synapse/process/process.h>
#include <synapse/interrupts/interrupt.h>

// Flag to indicate if scheduler is running
static bool scheduler_running = false;

static pid_t task_schedule_next_process()
{
  for (pid_t i = 0; i < SYNAPSE_MAX_PROCESSES; i++)
  {
    struct process* proc = process_get(i);
    if (proc && proc->task && proc->task->state == TASK_STATE_READY)
    {
      return i;
    }
  }

  return -ENOENT;
}

/**
 * @brief Timer interrupt handler for scheduling
 * 
 * @param int_frame Interrupt frame pointer
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 9 Apr 2025
 */
int scheduler_timer_handler(struct interrupt_frame* int_frame)
{
  if (!scheduler_running)
  {
    return EOK;
  }

  uart_send_string("TIMER IRQ scheduler!\n");

  // Save current task state if one is running
  struct task* current = task_current();
  if (current != NULL)
  {
    task_save_state(current, int_frame);

    // Set task back to ready state
    if (current->state == TASK_STATE_RUNNING)
    {
      current->state = TASK_STATE_READY;
    }
  }

  // Schedule next task
  pid_t next_pid = task_schedule_next_process();
  process_switch(next_pid);
  
  return EOK;
}

/**
 * @brief Initialize the scheduler
 * 
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 9 Apr 2025
 */
int scheduler_init()
{
  // Initialize the timer
  int res = timer_init();
  if (res < 0)
  {
    return res;
  }

  // Register timer interrupt handler
  res = timer_register_handler(scheduler_timer_handler);
  if (res < 0)
  {
    return res;
  }

  // Set timer interval
  res = timer_set_interval(SCHEDULER_TICKS_MS);
  if (res < 0)
  {
    return res;
  }

  return EOK;
}

/**
 * @brief Start yje scheduler
 * 
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 9 Apr 2025
 */
int scheduler_start()
{
  // Step 1: Enable timer (part of scheduler_start)
  uart_send_string("Enabling timer...\n");
  int res = timer_enable();
  if (res < 0)
  {
    uart_send_string("Failed to enable timer\n");
    return res;
  }
  uart_send_string("Timer enabled successfully\n");

  // Step 2: Enable global interrupts (part of scheduler_start)
  uart_send_string("Enabling global interrupts...\n");
  res = interrupt_enable_all();
  if (res < 0)
  {
    uart_send_string("Failed to enable global interrupts\n");
    timer_disable();
    return res;
  }
  uart_send_string("Global interrupts enabled successfully\n");

  // Step 3: Set scheduler as running
  uart_send_string("Setting scheduler as running...\n");
  scheduler_running = true;
  uart_send_string("Scheduler marked as running\n");

  // Step 4: Run first task
  uart_send_string("Running first task...\n");
  res = task_run_first_ever_task();
  if (res < 0)
  {
    uart_send_string("Failed to run first task\n");
    scheduler_running = false;
    timer_disable();
    interrupt_disable_all();
    return res;
  }
  
  return EOK;
}

/**
 * @brief Stop the scheduler
 * 
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 9 Arp 2025
 */
int scheduler_stop()
{
  // Disable timer interrupts
  int res = timer_disable();
  if (res < 0)
  {
    return res;
  }

  // Mark scheduler as stopped
  scheduler_running = false;

  return EOK;
}

/**
 * @brief Check if scheduler is running
 * 
 * @return true If the scheduler is running
 * @return false If the scheduler is not running
 * 
 * @author Fedi Nabli
 * @date 9 Apr 2025
 */
bool scheduler_is_running()
{
  return scheduler_running;
}

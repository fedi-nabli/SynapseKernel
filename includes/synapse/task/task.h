/*
 * task.h - Task management structures and functions for AArch64
 *
 * Author: Fedi Nabli
 * Date: 31 Mar 2025
 * Last Modified: 31 Mar 2025
 */

#ifndef __SYNAPSE_TASK_TASK_H_
#define __SYNAPSE_TASK_TASK_H_

#include <synapse/bool.h>
#include <synapse/types.h>

struct process;
struct interrupt_frame;

/* Task States */
#define TASK_STATE_NEW      0
#define TASK_STATE_READY    1
#define TASK_STATE_RUNNING  2
#define TASK_STATE_BLOCKED  3
#define TASK_STATE_FINISHED 4

/* Task Priorities */
#define TASK_PRIORITY_LOW     0
#define TASK_PRIORITY_NORMAL  1
#define TASK_PRIORITY_HIGH    2

/* Task structures  */
struct task_registers
{
  reg_t x0;
  reg_t x1;
  reg_t x2;
  reg_t x3;
  reg_t x4;
  reg_t x5;
  reg_t x6;
  reg_t x7;
  reg_t x8;
  reg_t x9;
  reg_t x10;
  reg_t x11;
  reg_t x12;
  reg_t x13;
  reg_t x14;
  reg_t x15;
  reg_t x16;
  reg_t x17;
  reg_t x18;
  reg_t x19;
  reg_t x20;
  reg_t x21;
  reg_t x22;
  reg_t x23;
  reg_t x24;
  reg_t x25;
  reg_t x26;
  reg_t x27;
  reg_t x28;
  reg_t x29; // Frame pointer
  reg_t x30; // Link register

  // Special register
  reg_t sp; // Stack pointer (x31)
  reg_t pc; // Program counter
  reg_t spsr_el1; // Save Program Status Register for EL1
  reg_t elr_el1; // Exception Link register for EL1
};

struct task
{
  // Identity
  tid_t id; // Task ID

  // State
  uint8_t state; // Ready, running, blocked, new or finished
  uint8_t priority; // Scheduling priority

  struct task_registers registers;

  struct process* process; // Owning process
  struct task* next;
  struct task* prev;
};

// Task context assembly functions
void task_save_context();
void task_restore_context(struct task* task);

/**
 * @brief Get current running task
 * 
 * @return struct task* Current task, NULL if none is running
 * 
 * @author Fedi Nabli
 * @date 31 Mar 2025
 */
struct task* task_current();

/**
 * @brief Create a new task
 * 
 * @return struct task* New task, NULL on failure
 * 
 * @author Fedi Nabli
 * @date 31 Mar 2025
 */
struct task* task_new(uint8_t task_priority);

/**
 * @brief Free task and remove from list
 * 
 * @param task Task to free
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 31 Mar 2025
 */
int task_free(struct task* task);

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
int task_save_state(struct task* task, struct interrupt_frame* int_frame);

/**
 * @brief Save current task state
 * 
 * @return int EOK on success, negative error on failure
 * 
 * @author Fedi Nabli
 * @date 31 Mar 2025
 */
int task_current_save_state();

/**
 * @brief Switch to a different task
 * 
 * @param task Task to switch to
 * @return int negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 31 Mar 2025
 */
int task_switch(struct task* task);

/**
 * @brief Schedule next task to run
 * 
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 31 Mar 2025
 */
int task_schedule();

/**
 * @brief Run the first task in the system
 * 
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 31 Mar 2025
 */
int task_run_first_ever_task();

/**
 * @brief Return from task to the kernel
 * 
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 31 Mar 2025
 */
int task_return();

/**
 * @brief Block current task
 * 
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 31 Mar 2025
 */
int task_block();

/**
 * @brief Unblock a task
 * 
 * @param task Task to unblock
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 31 Mar 2025
 */
int task_unblock(struct task* task);

#endif

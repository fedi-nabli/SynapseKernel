/*
 * svc.c - SVC (Supervisor Call) implementation for AArch64
 *
 * Author: Fedi Nabli
 * Date: 8 Apr 2025
 * Last Modified: 8 Apr 2025
 */

#include "svc.h"

#include <uart.h>

#include <synapse/bool.h>
#include <synapse/types.h>
#include <synapse/status.h>

#include <synapse/interrupts/interrupt.h>

// Global SVC handler
static SVC_HANDLER global_svc_handler = NULL;

/**
 * @brief Initialize SVC handling
 * 
 * @param handler Function to handle SVC calls
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 8 Apr 2025
 */
int svc_init(SVC_HANDLER handler)
{
  if (!handler)
  {
    return -EINVARG;
  }

  // Set global handler
  global_svc_handler = handler;

  return EOK;
}

/**
 * @brief User-space wrapper for SVC calls
 * 
 * @param syscall_num System call number
 * @param arg1 First argument
 * @param arg2 Second argument
 * @param arg3 Third argument
 * @param arg4 Forth argument
 * @return int System call results
 * 
 * @author Fedi Nabli
 * @date 8 Apr 2025
 */
int svc_call(int syscall_num, long arg1, long arg2, long arg3, long arg4)
{
  int result;

  // AArch64 inline assembly for SVC instruction
  __asm__ volatile(
    "mov x0, %1\n" // syscall number
    "mov x1, %2\n" // arg1
    "mov x2, %3\n" // arg2
    "mov x3, %4\n" // arg3
    "mov x4, %5\n" // arg4
    "svc #0\n" // SVC instruction with immediate value 0
    "mov %0, x0\n" // Get result from x0
    : "=r" (result)
    : "r" (syscall_num), "r" (arg1), "r" (arg2), "r" (arg3), "r" (arg4)
    : "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7"
  );

  return result;
}

/**
 * @brief C handler fo r SVC (Supervisor Call) interrupts
 * 
 * @param svc_num SVC number from instruction
 * @param int_frame Interrupt frame containing register values
 * @return int System call results
 * 
 * @author Fedi Nabli
 * @date 8 Apr 2025
 */
int svc_c_handler(int svc_num, struct interrupt_frame* int_frame)
{
  // SVC number is the system syscall number
  int syscall_num = svc_num;

  // Get arguments from registers (AArch64 calling convention uses x0-x7)
  long arg1 = int_frame->x1; // x0 contains syscall number
  long arg2 = int_frame->x2;
  long arg3 = int_frame->x3;
  long arg4 = int_frame->x4;

  // Call system call handler
  int result = EOK;
  if (global_svc_handler)
  {
    result = global_svc_handler(syscall_num, arg1, arg2, arg3, arg4);
  }
  else
  {
    result = -EINVSYSCALL;
  }

  // Store result in x0 for return to user
  int_frame->x0 = result;

  return result;
}

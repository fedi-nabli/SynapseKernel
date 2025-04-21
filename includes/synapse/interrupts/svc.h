/*
 * svc.h - SVC (Supervisor Call) handling for AArch64
 *
 * Author: Fedi Nabli
 * Date: 8 Apr 2025
 * Last Modified: 8 Apr 2025
 */

#ifndef __SYNAPSE_INTERRUPTS_SVC_H_
#define __SYNAPSE_INTERRUPTS_SVC_H_

#include <synapse/bool.h>
#include <synapse/types.h>
#include <synapse/interrupts/interrupt.h>

// SVC handler types
typedef int (*SVC_HANDLER)(int syscall_num, long arg1, long arg2, long arg3, long arg4);

// SVC Management handler
/**
 * @brief Initialize SVC handling
 * 
 * @param handler Function to handle SVC calls
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 8 Apr 2025
 */
int svc_init(SVC_HANDLER handler);

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
int svc_call(int syscall_num, long arg1, long arg2, long arg3, long arg4);

// Called from assembly
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
int svc_c_handler(int svc_num, struct interrupt_frame* int_frame);

#endif
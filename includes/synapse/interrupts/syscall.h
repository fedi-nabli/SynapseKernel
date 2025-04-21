/*
 * syscall.h - System call interface for AArch64
 *
 * Author: Fedi Nabli
 * Date: 8 Apr 2025
 * Last Modified: 9 Apr 2025
 */

#ifndef __SYNAPSE_INTERRUPTS_SYSCALL_H_
#define __SYNAPSE_INTERRUPTS_SYSCALL_H_

#include <synapse/bool.h>
#include <synapse/types.h>

#include <synapse/interrupts/interrupt.h>

// System call numbers
#define SYSCALL_PROCESS_EXIT      0
#define SYSCALL_PROCESS_MALLOC    1
#define SYSCALL_PROCESS_FREE      2
#define SYSCALL_PROCESS_GET_ARGS  3
#define SYSCALL_PRINT_CHAR        4
#define SYSCALL_PRINT_STRING      5
#define SYSCALL_MAX               6

/**
 * @brief Initialize system call interface
 * 
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 8 Apr 2025
 */
int syscall_init();

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
int syscall_handler(int syscall_num, long arg1, long arg2, long arg3, long arg4);

// /**
//  * @brief C handler for SVC (Supervisor Call) interrupts
//  * 
//  * @param svc_num SVC number from instruction
//  * @param int_frame Interrupt frame containing register values
//  * @return int System call result
//  * 
//  * @author Fedi Nabli
//  * @date 8 Apr 2025
//  */
// int svc_c_handler(int svc_num, struct interrupt_frame* int_frame);

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
int syscall(int syscall_num, long arg1, long arg2, long arg3, long arg4);

/**
 * @brief Process exit system call wrapper
 * 
 * @param exit_code Exit code
 * 
 * @author Fedi Nabli
 * @date 9 Apr 2025
 */
void syscall_exit(int exit_code);

/**
 * @brief Process memory allocation system call wrapper
 * 
 * @param size Size to allocate
 * @return void* Allocated memory, NULL on failure
 * 
 * @author Fedi Nabli
 * @date 9 Apr 2025
 */
void* syscall_malloc(size_t size);

/**
 * @brief Process memory free system call wrapper
 * 
 * @param ptr Pointer to free
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 9 Apr 2025
 */
int syscall_free(void* ptr);

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
int syscall_get_args(int* argc, char*** argv);

/**
 * @brief Print character system call wrapper
 * 
 * @param c Character to print
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 9 Apr 2025
 */
int syscall_print_char(char c);

/**
 * @brief Print string system call wrapper
 * 
 * @param str String to print
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 9 Apr 2025
 */
int syscall_print_string(const char* str);

#endif
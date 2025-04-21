/*
 * scheduler.h - Task scheduler functions for AArch64
 *
 * Author: Fedi Nabli
 * Date: 9 Apr 2025
 * Last Modified: 9 Apr 2025
 */

#ifndef __SYNAPSE_SCHEDULER_H_
#define __SYNAPSE_SCHEDULER_H_

#include <synapse/bool.h>
#include <synapse/types.h>

#include <synapse/interrupts/interrupt.h>

/**
 * @brief Timer interrupt handler for scheduling
 * 
 * @param int_frame Interrupt frame pointer
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 9 Apr 2025
 */
int scheduler_timer_handler(struct interrupt_frame* int_frame);

/**
 * @brief Initialize the scheduler
 * 
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 9 Apr 2025
 */
int scheduler_init();

/**
 * @brief Start yje scheduler
 * 
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 9 Apr 2025
 */
int scheduler_start();

/**
 * @brief Stop the scheduler
 * 
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 9 Arp 2025
 */
int scheduler_stop();

/**
 * @brief Check if scheduler is running
 * 
 * @return true If the scheduler is running
 * @return false If the scheduler is not running
 * 
 * @author Fedi Nabli
 * @date 9 Apr 2025
 */
bool scheduler_is_running();

#endif
/*
 * timer.h - System timer management for AArch64
 *
 * Author: Fedi Nabli
 * Date: 8 Apr 2025
 * Last Modified: 8 Apr 2025
 */

#ifndef __SYNAPSE_TIMER_H_
#define __SYNAPSE_TIMER_H_

#include <synapse/bool.h>
#include <synapse/types.h>

#include <synapse/interrupts/interrupt.h>

/**
* ARM Generic Timer registers
* These are accessed via system registers in AArch64
*/
// Timer IRQ number in GIC
#define TIMER_IRQ 30 // PPI Interrupt
 
// Timer management functions
/**
* @brief Timer interrupt handler
* 
* @param int_frame Interrupt frame
* @return int Handler result
* 
* @author Fedi Nabli
* @date 8 Apr 2025
*/
int timer_irq_handler(struct interrupt_frame* int_frame);

 /**
 * @brief Initialize system timer for AArch64
 * 
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 8 Apr 2025
 */
int timer_init();
 
/**
* @brief Register a timer handler function
* 
* @param handler Function to call on timer interrupts
* @return int EOK on success, negative error code on failure
* 
* @author Fedi Nabli
* @date 8 Apr 2025
*/
int timer_register_handler(INTERRUPT_HANDLER handler);

/**
* @brief Unregister the timer handler
* 
* @return int EOK on success, negative error code on failure
* 
* @author Fedi Nabli
* @date 8 Apr 2025
*/
int timer_unregister_handler();

/**
* @brief Set timer interval in milliseconds
* 
* @param ms Interval in milliseconds
* @return int EOK on success, negative error code on failure
* 
* @author Fedi Nabli
* @date 8 Apr 2025
*/

int timer_set_interval(uint32_t ms);

/**
* @brief Enable the timer
* 
* @return int EOK on success, negative error code on failure
* 
* @author Fedi Nabli
* @date 8 Apr 2025
*/
int timer_enable();

/**
* @brief Disable the timer
* 
* @return int EOK on success, negative error code on failure
* 
* @author Fedi Nabli
* @date 8 Apr 2025
*/
int timer_disable();

/**
* @brief Get the number of timer ticks
* 
* @return uint64_t Current tick cout
* 
* @author Fedi Nabli
* @date 8 Apr 2025
*/
uint64_t timer_get_ticks();

/**
* @brief Get elapsed milliseconds since timer start
* 
* @return uint64_t Milliseconds since timer start
* 
* @author Fedi Nabli
* @date 8 Apr 2025
*/
uint64_t timer_get_ms();

#endif
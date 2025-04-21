/*
 * timer.c - System timer implementation for AArch64
 *
 * Author: Fedi Nabli
 * Date: 8 Apr 2025
 * Last Modified: 8 Apr 2025
 */

#include "timer.h"

#include <uart.h>

#include <kernel/config.h>

#include <synapse/bool.h>
#include <synapse/types.h>
#include <synapse/status.h>

#include <synapse/task/task.h>
#include <synapse/interrupts/interrupt.h>

// Timer state
static bool timer_initialized = false;
static INTERRUPT_HANDLER timer_handler = NULL;
static uint64_t timer_ticks = 0;
static uint32_t timer_interval_ms = 0;

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

/**
 * @brief Get the current counter value from the system timer
 * 
 * @return uint64_t Current counter value
 * 
 * @author Fedi Nabli
 * @date 8 Apr 2025
 */
static inline uint64_t read_cntpct_el0()
{
  uint64_t result;
  __asm__ volatile("mrs %0, cntpct_el0" : "=r" (result));
  return result;
}

/**
 * @brief Get the timer frequency
 * 
 * @return uint64_t Timer frequency in Hz
 * 
 * @author Fedi Nabli
 * @date 8 Apr 2025
 */
static inline uint64_t read_cntfrq_el0()
{
  uint64_t result;
  __asm__ volatile("mrs %0, cntfrq_el0" : "=r" (result));
  return result;
}

/**
 * @brief Set the timer compare value
 * 
 * @param value Compare value
 * 
 * @author Fedi Nabli
 * @date 8 Apr 2025
 */
static inline void write_cntp_cval_el0(uint64_t value)
{
  __asm__ volatile("msr cntp_cval_el0, %0" :: "r" (value));
}

/**
 * @brief Get the timer control register
 * 
 * @return uint32_t Control register value
 * 
 * @author Fedi Nabli
 * @date 8 Apr 2025
 */
static inline uint32_t read_cntp_ctl_el0()
{
  uint32_t result;
  __asm__ volatile("mrs %0, cntp_ctl_el0" : "=r" (result));
  return result;
}

/**
 * @brief Set the timer control register
 * 
 * @param value Control register value
 * 
 * @author Fedi Nabli
 * @date 8 Apr 2025
 */
static inline void write_cntp_ctl_el0(uint32_t value)
{
  __asm__ volatile("msr cntp_ctl_el0, %0" :: "r" (value));
}

/**
 * @brief Timer interrupt handler
 * 
 * @param int_frame Interrupt frame
 * @return int Handler result
 * 
 * @author Fedi Nabli
 * @date 8 Apr 2025
 */
int timer_irq_handler(struct interrupt_frame* int_frame)
{
  uart_send_string(">> timer_irq_handler() fired!\n");
  uart_send_string("TIMER IRQ!\n");
  
  // Increment tick counter
  timer_ticks++;

  // Set next timer compare value
  uint64_t current = read_cntpct_el0();
  uint64_t frequency = read_cntfrq_el0();
  uint64_t interval = (frequency * timer_interval_ms) / 1000;
  write_cntp_cval_el0(current + interval);

  // Call registered handler if any
  if (timer_handler)
  {
    return timer_handler(int_frame);
  }

  return EOK;
}

/**
 * @brief Initialize system timer for AArch64
 * 
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 8 Apr 2025
 */
int timer_init(void)
{
  if (timer_initialized)
  {
    return EOK;
  }
  
  // Register IRQ handler with GIC
  int res = interrupt_register_handler(TIMER_IRQ, timer_irq_handler);
  if (res == EOK)
  {
    uart_send_string("timer_init: handler registered!\n");
  }
  else
  {
    uart_send_string("timer_init: handler REGISTRATION FAILED!\n");
    return res;
  }
  
  // Initialize the ARM Generic Timer
  // Disable timer initially
  write_cntp_ctl_el0(0);
  
  // Set timer frequency in CNTFRQ_EL0 if not already set
  uint64_t freq = read_cntfrq_el0();
  if (freq == 0)
  {
    __asm__ volatile("msr cntfrq_el0, %0" :: "r" (CPU_FREQ_HZ));
  }
  
  timer_initialized = true;
  return EOK;
}

/**
 * @brief Register a timer handler function
 * 
 * @param handler Function to call on timer interrupts
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 8 Apr 2025
 */
int timer_register_handler(INTERRUPT_HANDLER handler)
{
  if (!timer_initialized)
  {
    return -ENOTREADY;
  }

  timer_handler = handler;
  return EOK;
}

/**
 * @brief Unregister the timer handler
 * 
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 8 Apr 2025
 */
int timer_unregister_handler()
{
  if (!timer_initialized)
  {
    return -ENOTREADY;
  }

  timer_handler = NULL;
  return EOK;
}

/**
 * @brief Set timer interval in milliseconds
 * 
 * @param ms Interval in milliseconds
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 8 Apr 2025
 */
int timer_set_interval(uint32_t ms)
{
  if (!timer_initialized)
  {
    return -ENOTREADY;
  }

  if (ms == 0)
  {
    return -EINVARG;
  }

  timer_interval_ms = ms;

  // Disable timer before changing interval
  write_cntp_ctl_el0(0);

  // Calculate and set compare value
  uint64_t current = read_cntpct_el0();
  uint64_t frequency = read_cntfrq_el0();
  uint64_t interval = (frequency * ms) / 1000;
  write_cntp_cval_el0(current + interval);

  return EOK;
}

/**
 * @brief Enable the timer
 * 
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 8 Apr 2025
 */
int timer_enable()
{
  if (!timer_initialized)
  {
    return -ENOTREADY;
  }

  // 1. Enable timer (CNTP_CTL_EL0.ENABLE = 1)
  write_cntp_ctl_el0(1);

  // 2. Unmask interrupts at CPU level
  __asm__ volatile("msr daifclr, #2"); // Clear I bit in DAIF (enable IRQ)

  // 3. Also make sure CNTPNS IRQ is unmasked (redundant safety)
  __asm__ volatile(
    "mov x0, #1\n"
    "msr cntp_ctl_el0, x0\n"  // Enable again to be safe
    "isb\n"
  );

  // 4. Enable timer interrupt in the GIC
  interrupt_enable(TIMER_IRQ);

  // Debug output
  uint32_t val;
  __asm__ volatile("mrs %0, cntp_ctl_el0" : "=r"(val));
  uart_send_string("CNT_CTL after enable: ");
  char buf[20];
  uint64_to_str(val, buf, sizeof(buf));
  uart_send_string(buf);
  uart_send_string("\n");

  return EOK;
}

/**
 * @brief Disable the timer
 * 
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 8 Apr 2025
 */
int timer_disable()
{
  if (!timer_initialized)
  {
    return -ENOTREADY;
  }

  // Disable timer
  // CNTP_CTL_EL0.ENABLE = 0
  write_cntp_ctl_el0(0);

  // Disable timer IRQ in GOC
  interrupt_disable(TIMER_IRQ);

  return EOK;
}

/**
 * @brief Get the number of timer ticks
 * 
 * @return uint64_t Current tick cout
 * 
 * @author Fedi Nabli
 * @date 8 Apr 2025
 */
uint64_t timer_get_ticks()
{
  return timer_ticks;
}

/**
 * @brief Get elapsed milliseconds since timer start
 * 
 * @return uint64_t Milliseconds since timer start
 * 
 * @author Fedi Nabli
 * @date 8 Apr 2025
 */
uint64_t timer_get_ms()
{
  if (timer_interval_ms == 0)
  {
    return 0;
  }

  return timer_ticks * timer_interval_ms;
}

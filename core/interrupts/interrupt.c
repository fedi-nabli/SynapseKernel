#include "interrupt.h"

#include <uart.h>

#include <kernel/config.h>

#include <synapse/bool.h>
#include <synapse/types.h>
#include <synapse/status.h>

#include <synapse/memory/memory.h>

// Interrupt handler table
static INTERRUPT_HANDLER interrupt_handlers[MAX_INTERRUPT_HANDLERS] = { 0 };

// Interrupt initialization status
static bool interrupt_initialized = false;

/**
 * @brief Initialize interrupt subsystem for AArch64
 * 
 * This function sets up the necessary configurations for the interrupt
 * system to operate correctly. It ensures that the interrupt controller
 * and related components are properly initialized.
 * 
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 28 Mar 2025
 */
int interrupt_init()
{
  if (interrupt_initialized)
  {
    return EOK;
  }

  // Clear all handlers
  memset(interrupt_handlers, 0, sizeof(interrupt_handlers));

  // Initialize GIC (Generic Interrupt Controller)

  // 1: Initialize GIC Distributor (GICD)
  GICD_CTLR = 0x00; // Disable GIC Distributor

  // Configure all interrupts to be level-triggered
  for (int i = 0; i < MAX_INTERRUPT_HANDLERS/16; i++)
  {
    GICD_ICFGR(i) = 0;
  }

  // Disbale all interrupts initially
  for (int i = 0; i < MAX_INTERRUPT_HANDLERS/32; i++)
  {
    GICD_ICENABLER(i) = 0xFFFFFFFF;
  }

  // Clean any pending interrupts
  for (int i = 0; i < MAX_INTERRUPT_HANDLERS/32; i++)
  {
    GICD_ISPENDR(i) = 0xFFFFFFFF;
  }

  GICD_CTLR = 0x1; // Enable GIC Distributor

  // 2: Initialize GIC CPU Interface (GICC)
  GICC_CTLR = 0x0; // Diable CPU Interface
  GICC_PMR = 0xFF; // No priority masking (lowest priority level)
  GICC_BPR = 0x0; // Use all priority bits for group integrity
  GICC_CTLR = 0x1; // Enable CPU Interface

  // 3: Set up exception vector table
  // This is done in vector.S and loaded during boot

  interrupt_initialized = true;

  return EOK;
}

/**
 * @brief Register an interrupt handler
 * 
 * @param interrupt_num Interrupt number to handle
 * @param handler Function to call when interrupt occurs
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 28 Mar 2025
 */
int interrupt_register_handler(uint32_t interrupt_num, INTERRUPT_HANDLER handler)
{
  if (!interrupt_initialized)
  {
    return -ENOTREADY;
  }

  if (interrupt_num >= MAX_INTERRUPT_HANDLERS || !handler)
  {
    return -EINVARG;
  }

  if (interrupt_handlers[interrupt_num] != NULL)
  {
    return -EINUSE; // Handler already registered
  }

  // Register handler
  interrupt_handlers[interrupt_num] = handler;

  return EOK;
}

/**
 * @brief Unregister an interrupt handler
 * 
 * @param interrupt_num Interrupt number to unregister
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 28 Mar 2025
 */
int interrupt_unregister_handler(uint32_t interrupt_num)
{
  if (!interrupt_initialized)
  {
    return -ENOTREADY;
  }

  if (interrupt_num >= MAX_INTERRUPT_HANDLERS)
  {
    return -EINVARG;
  }

  // Unregister handler
  interrupt_handlers[interrupt_num] = NULL;

  return EOK;
}

/**
 * @brief Enable a specific interrupt
 * 
 * @param interrupt_num Interrupt number to enable
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 28 Mar 2025
 */
int interrupt_enable(uint32_t interrupt_num)
{
  if (!interrupt_initialized)
  {
    return -ENOTREADY;
  }

  if (interrupt_num >= MAX_INTERRUPT_HANDLERS)
  {
    return -EINVARG;
  }

  // Enable the interrupt in GIC Distributor
  uint32_t reg_idx = interrupt_num / 32;
  uint32_t bit_offset = interrupt_num % 32;

  GICD_ISENABLER(reg_idx) = (1 << bit_offset);

  return EOK;
}

/**
 * @brief Disable a specific interrupt
 * 
 * @param interrupt_num Interrupt number to disable
 * @return int EOK on sucess, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 28 Mar 2025
 */
int interrupt_disable(uint32_t interrupt_num)
{
  if (!interrupt_initialized)
  {
    return -ENOTREADY;
  }

  if (interrupt_num >= MAX_INTERRUPT_HANDLERS)
  {
    return -EINVARG;
  }

  // Disable the interrupt in GIC Distributor
  uint32_t reg_idx = interrupt_num / 32;
  uint32_t bit_offset = interrupt_num % 32;

  GICD_ICENABLER(reg_idx) = (1 << bit_offset);

  return EOK;
}

/**
 * @brief Enable all interrupts (at CPU level)
 * 
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 28 Mar 2025
 */
int interrupt_enable_all()
{
  if (!interrupt_initialized)
  {
    return -ENOTREADY;
  }

  // Enable interrupts at CPU level
  __asm__ volatile("msr daifclr, #2"); // Clear I bit in DAIF

  return EOK;
}

/**
 * @brief Disable all interrupts (at CPU level)
 * 
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 28 Mar 2025
 */
int interrupt_disable_all()
{
  if (!interrupt_initialized)
  {
    return -ENOTREADY;
  }

  // Disable interrupts at CPU level
  __asm__ volatile("msr daifset, #2"); // Set I bit in DAIF

  return EOK;
}

/**
 * @brief Main IRQ handler called from exception vector
 * 
 * @param int_frame Interrupt frame with register values
 * @return int Handler result
 * 
 * @author Fedi Nabli
 * @date 28 Mar 2025
 */
int irq_handler(struct interrupt_frame* int_frame)
{
  if (!interrupt_initialized)
  {
    return -ENOTREADY;
  }

  // Ready interrupt ID from GIC CPU Interface
  uint32_t iar = GICC_IAR;
  uint32_t interrupt_id = iar & 0x3FF; // Interrupt ID is in the low 10 bits

  // Special IDs (1020, 1021, 1023) are not actual interrupts
  if (interrupt_id >= 1020)
  {
    return EOK;
  }

  // Call registered handler
  int res = EOK;
  if (interrupt_handlers[interrupt_id] != NULL)
  {
    res = interrupt_handlers[interrupt_id](int_frame);
  }

  // Signal End of Interrupt to GIC
  GICC_EOIR = iar;

  return res;
}

/**
 * @brief Handle IRQ exception from EL1
 * 
 * @param int_frame Interrupt frame
 * @return int Handler result
 * 
 * @author Fedi Nabli
 * @date 28 Mar 2025
 */
int el1_irq_handler(struct interrupt_frame* int_frame)
{
  // Just call the generic IRQ handler
  return irq_handler(int_frame);
}

/**
 * @brief Handle IRQ exception from EL0
 * 
 * @param int_frame Interrupt frame
 * @return int Handler result
 * 
 * @author Fedi Nabli
 * @date 28 Mar 2025
 */
int el0_irq_handler(struct interrupt_frame* int_frame)
{
  // TODO: Save current task stateif one is running

  // Handle the IRQ
  int res = irq_handler(int_frame);

  // TODO: Check if we need to schedule after handling the IRQ

  return res;
}

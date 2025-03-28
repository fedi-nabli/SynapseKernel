#ifndef __SYNAPSE_INTERRUPT_H_
#define __SYNAPSE_INTERRUPT_H_

#include <synapse/bool.h>
#include <synapse/types.h>

// ARM GICv2 constants (Generic Interrupt Controller)
#define GIC_BASE_ADDRESS ((uintptr_t)0x08000000)

// GIC Distriibutor registers
#define GICD_BASE           (GIC_BASE_ADDRESS + 0x1000)
#define GICD_CTLR           (*((volatile uint32_t*)(GICD_BASE + 0x000)))
#define GICD_TYPER          (*((volatile uint32_t*)(GICD_BASE + 0x004)))
#define GICD_IIDR           (*((volatile uint32_t*)(GICD_BASE + 0x008)))

#define GICD_ISENABLER(n)   (*((volatile uint32_t*)(GICD_BASE + 0x100 + ((n) * 4))))
#define GICD_ICENABLER(n)   (*((volatile uint32_t*)(GICD_BASE + 0x180 + ((n) * 4))))
#define GICD_ISPENDR(n)     (*((volatile uint32_t*)(GICD_BASE + 0x200 + ((n) * 4))))
#define GICD_ICPENDR(n)     (*((volatile uint32_t*)(GICD_BASE + 0x280 + ((n) * 4))))
#define GICD_ICFGR(n)       (*((volatile uint32_t*)(GICD_BASE + 0xC00 + ((n) * 4))))

// GIC CPU Interface registers
#define GICC_BASE           (GIC_BASE_ADDRESS + 0x2000)
#define GICC_CTLR           (*((volatile uint32_t*)(GICC_BASE + 0x000)))
#define GICC_PMR            (*((volatile uint32_t*)(GICC_BASE + 0x004)))
#define GICC_BPR            (*((volatile uint32_t*)(GICC_BASE + 0x008)))
#define GICC_IAR            (*((volatile uint32_t*)(GICC_BASE + 0x00C)))
#define GICC_EOIR           (*((volatile uint32_t*)(GICC_BASE + 0x010)))

struct interrupt_frame
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

  reg_t sp; // Stack pointer (x31)
  reg_t elr_el1; // Exception Link Register (return address)
  reg_t spsr_el1; // Saved processor state
};

// Interrupt handler type
typedef int (*INTERRUPT_HANDLER)(struct interrupt_frame* int_frame);

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
int interrupt_init();

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
int interrupt_register_handler(uint32_t interrupt_num, INTERRUPT_HANDLER handler);

/**
 * @brief Unregister an interrupt handler
 * 
 * @param interrupt_num Interrupt number to unregister
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 28 Mar 2025
 */
int interrupt_unregister_handler(uint32_t interrupt_num);

/**
 * @brief Enable a specific interrupt
 * 
 * @param interrupt_num Interrupt number to enable
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 28 Mar 2025
 */
int interrupt_enable(uint32_t interrupt_num);

/**
 * @brief Disable a specific interrupt
 * 
 * @param interrupt_num Interrupt number to disable
 * @return int EOK on sucess, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 28 Mar 2025
 */
int interrupt_disable(uint32_t interrupt_num);

/**
 * @brief Enable all interrupts (at CPU level)
 * 
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 28 Mar 2025
 */
int interrupt_enable_all();

/**
 * @brief Disable all interrupts (at CPU level)
 * 
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 28 Mar 2025
 */
int interrupt_disable_all();

/**
 * @brief Main IRQ handler called from exception vector
 * 
 * @param int_frame Interrupt frame with register values
 * @return int Handler result
 * 
 * @author Fedi Nabli
 * @date 28 Mar 2025
 */
int irq_handler(struct interrupt_frame* int_frame);

/**
 * @brief Handle IRQ exception from EL1
 * 
 * @param int_frame Interrupt frame
 * @return int Handler result
 * 
 * @author Fedi Nabli
 * @date 28 Mar 2025
 */
int el1_irq_handler(struct interrupt_frame* int_frame);

/**
 * @brief Handle IRQ exception from EL0
 * 
 * @param int_frame Interrupt frame
 * @return int Handler result
 * 
 * @author Fedi Nabli
 * @date 28 Mar 2025
 */
int el0_irq_handler(struct interrupt_frame* int_frame);

#endif
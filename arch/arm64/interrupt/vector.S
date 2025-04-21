/*
 * vector.S - AArch64 exception vector table with debugging handlers
 *
 * This file defines the full exception vector table for AArch64. Each entry
 * prints a diagnostic message via uart_send_string, then either calls a 
 * known function (irq_handler_entry, svc_handler) or loops forever (exception_hang).
 *
 * Author: Fedi Nabli
 * Date: 26 Feb 2025
 * Last Modified: <provide today's date>
 */

 .section ".vector", "ax"
 .align 11               // AArch64 requires 2KB alignment for the vector table
 .global vector_table

 /*
  * Layout of the AArch64 Vector Table (one for each of the four
  * Exception levels, but typically we only define the EL1 table here).
  *
  * For each of the four stack pointer contexts (SP0 vs. SPx) at each of
  * the four possible exception levels, we have 4 entries:
  *
  *  0) Synchronous
  *  1) IRQ
  *  2) FIQ
  *  3) SError
  *
  * This totals 16 entries. Then we also define separate placeholders
  * for AArch64 vs AArch32 lower EL exceptions if needed.
  */

vector_table:
 /* -----------------------------
  *  EL1t (SP_EL0) Vector entries
  * -----------------------------
  */
 .align 7
 b  el1_sync_sp0   // 0: Synchronous
 .align 7
 b  el1_irq_sp0    // 1: IRQ
 .align 7
 b  el1_fiq_sp0    // 2: FIQ
 .align 7
 b  el1_error_sp0  // 3: SError

 /* -----------------------------
  *  EL1h (SP_EL1) Vector entries
  * -----------------------------
  */
 .align 7
 b  el1_sync_spx
 .align 7
 b  el1_irq_spx
 .align 7
 b  el1_fiq_spx
 .align 7
 b  el1_error_spx

 /* -----------------------------------------------------------
  *  EL0 using AArch64 Vector entries (when taking exceptions
  *  from lower EL to EL1 in AArch64 mode)
  * -----------------------------------------------------------
  */
 .align 7
 b  el0_sync_a64
 .align 7
 b  el0_irq_a64
 .align 7
 b  el0_fiq_a64
 .align 7
 b  el0_error_a64

 /* -----------------------------------------------------------
  *  EL0 using AArch32 Vector entries (when taking exceptions
  *  from lower EL to EL1 in AArch32 mode) - typically not used
  * -----------------------------------------------------------
  */
 .align 7
 b  el0_sync_a32
 .align 7
 b  el0_irq_a32
 .align 7
 b  el0_fiq_a32
 .align 7
 b  el0_error_a32

/* =======================================================================
 * EL1 with SP_EL0 handlers (EL1t)
 * =======================================================================
 */
el1_sync_sp0:
  adrp x0, str_el1_sync_sp0
  add  x0, x0, :lo12:str_el1_sync_sp0
  bl   uart_send_string
  b    exception_hang

el1_irq_sp0:
  adrp x0, str_el1_irq_sp0
  add  x0, x0, :lo12:str_el1_irq_sp0
  bl   uart_send_string
  b    exception_hang

el1_fiq_sp0:
  adrp x0, str_el1_fiq_sp0
  add  x0, x0, :lo12:str_el1_fiq_sp0
  bl   uart_send_string
  b    exception_hang

el1_error_sp0:
  adrp x0, str_el1_error_sp0
  add  x0, x0, :lo12:str_el1_error_sp0
  bl   uart_send_string
  b    exception_hang

/* =======================================================================
 * EL1 with SP_EL1 handlers (EL1h)
 * These are typically used by the kernel once it sets SP_EL1.
 * =======================================================================
 */
el1_sync_spx:
  // Print a debug message
  adrp x0, str_el1_sync_spx
  add  x0, x0, :lo12:str_el1_sync_spx
  bl   uart_send_string

  // Check ESR to see if it was an SVC from EL1
  mrs x0, esr_el1
  lsr x0, x0, #26           // extract EC (Exception Class)
  cmp x0, #0x15             // 0x15 = SVC
  beq el1_svc_handler

  b exception_hang

el1_svc_handler:
  adrp x0, str_el1_svc
  add  x0, x0, :lo12:str_el1_svc
  bl   uart_send_string
  b    exception_hang

el1_irq_spx:
  // Normal IRQ taken from EL1, using SP_EL1
  // We'll jump to a known assembly stub that saves regs and calls C
  bl   irq_handler_entry
  // If the IRQ handler returns, do we resume?
  // Usually we 'eret' from the low-level code. But if we just do 'b ret_from_irq', 
  // that must do the final eret.
  // For minimal debugging, just do:
  b    exception_hang

el1_fiq_spx:
  adrp x0, str_el1_fiq_spx
  add  x0, x0, :lo12:str_el1_fiq_spx
  bl   uart_send_string
  b    exception_hang

el1_error_spx:
  adrp x0, str_el1_error_spx
  add  x0, x0, :lo12:str_el1_error_spx
  bl   uart_send_string
  b    exception_hang

/* =======================================================================
 * EL0 AArch64 handlers (when user code triggers an exception to EL1)
 * Typically handle SVC calls (syscalls), IRQ from user mode, etc.
 * =======================================================================
 */
el0_sync_a64:
  adrp x0, str_el0_sync_a64
  add  x0, x0, :lo12:str_el0_sync_a64
  bl   uart_send_string

  // Check ESR to see if it was an SVC from EL0
  mrs x0, esr_el1
  lsr x0, x0, #26        // extract EC
  cmp x0, #0x15          // 0x15 = SVC
  beq el0_svc_handler
  mrs x0, esr_el1        // ESR value → x0
  bl dump_esr_el1
  b   exception_hang

el0_svc_handler:
  // If you want the SVC number:
  // mrs x0, esr_el1
  // and x0, x0, #0xFFFF
  // Then call a C function with x0 as the SVC number:
  // bl svc_handler

  // For debugging:
  adrp x0, str_el0_svc
  add  x0, x0, :lo12:str_el0_svc
  bl   uart_send_string

  // Actually call your assembly code for SVC, or handle in C:
  b svc_handler
  b  exception_hang

el0_irq_a64:
  // IRQ from user mode
  bl   irq_handler_entry
  b    exception_hang

el0_fiq_a64:
    adrp x0, str_el0_fiq_a64
    add  x0, x0, :lo12:str_el0_fiq_a64
    bl   uart_send_string
    b    exception_hang

el0_error_a64:
  adrp x0, str_el0_error_a64
  add  x0, x0, :lo12:str_el0_error_a64
  bl   uart_send_string
  b    exception_hang

/* =======================================================================
 * EL0 AArch32 handlers
 * Typically not used in a pure AArch64 environment
 * =======================================================================
 */
el0_sync_a32:
el0_irq_a32:
el0_fiq_a32:
el0_error_a32:
  adrp x0, str_el0_aarch32
  add  x0, x0, :lo12:str_el0_aarch32
  bl   uart_send_string
  b    exception_hang

/* =======================================================================
 * Utility placeholders
 * =======================================================================
 */

exception_hang:
  // Print the "System halted" message
  adrp x0, str_exception_hang
  add  x0, x0, :lo12:str_exception_hang
  bl   uart_send_string
1:
  wfe
  b 1b

continue_boot:
  adrp x0, str_continue_boot
  add  x0, x0, :lo12:str_continue_boot
  bl   uart_send_string
  b  next_stage

next_stage:
  // Normally you'd jump to kernel_main or similar next code
  b exception_hang

/* =======================================================================
 * Read-only debug strings
 * =======================================================================
 */
.section ".rodata"

/* EL1t (SP_EL0) messages */
str_el1_sync_sp0:
  .asciz "EXCEPTION: EL1 Synchronous (SP_EL0)\n"
str_el1_irq_sp0:
  .asciz "EXCEPTION: EL1 IRQ (SP_EL0)\n"
str_el1_fiq_sp0:
  .asciz "EXCEPTION: EL1 FIQ (SP_EL0)\n"
str_el1_error_sp0:
  .asciz "EXCEPTION: EL1 SError (SP_EL0)\n"

/* EL1h (SP_EL1) messages */
str_el1_sync_spx:
  .asciz "EXCEPTION: EL1 Synchronous (SP_ELx)\n"
str_el1_svc:
  .asciz "EXCEPTION: EL1 SVC (unexpected)\n"
str_el1_irq_spx:
  .asciz "EXCEPTION: EL1 IRQ (SP_ELx)\n"
str_el1_fiq_spx:
  .asciz "EXCEPTION: EL1 FIQ (SP_ELx)\n"
str_el1_error_spx:
  .asciz "EXCEPTION: EL1 SError (SP_ELx)\n"

/* EL0 AArch64 messages */
str_el0_sync_a64:
  .asciz "EXCEPTION: EL0/A64 Synchronous\n"
str_el0_svc:
  .asciz "EXCEPTION: EL0/A64 SVC\n"
str_el0_irq_a64:
  .asciz "EXCEPTION: EL0/A64 IRQ\n"
str_el0_fiq_a64:
  .asciz "EXCEPTION: EL0/A64 FIQ\n"
str_el0_error_a64:
  .asciz "EXCEPTION: EL0/A64 SError\n"

/* EL0 AArch32 message */
str_el0_aarch32:
  .asciz "EXCEPTION: EL0/A32 Exception (unexpected)\n"

/* Hang / Boot messages */
str_exception_hang:
  .asciz "System halted due to exception\n"
str_continue_boot:
  .asciz "Continuing boot process...\n"

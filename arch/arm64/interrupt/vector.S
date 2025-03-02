/*
 * vector.S - AArch64 exception vector table
 *
 * This file defines the exception vector table for AArch64.
 * The vector table must be 2KB aligned and consist of 16 entries,
 * each 128 bytes (32 instructions) apart.
 *
 * Author: Fedi Nabli
 * Date: 26 Feb 2025
 * Last Modified: 26 Feb 2025
 */

.section ".vector", "ax"
.align 11 // 2 ^ 11 = 2048 byte alignment for AArch64 vector table

.global vector_table

vector_table:
  /* Current Exception Level with SP_EL0 */
  // Synchronous
  .align 7
  b el1_sync_sp0
  // IRQ
  .align 7
  b el1_irq_sp0
  // FIQ
  .align 7
  b el1_fiq_sp0
  // SError
  .align 7
  b el1_error_sp0

  /* Current Exception Level with SP_ELx */
  // Synchronous
  .align 7
  b el1_sync_spx
  // IRQ
  .align 7
  b el1_irq_spx
  // FIQ
  .align 7
  b el1_fiq_spx
  // SError
  .align 7
  b el1_error_spx

  /* Lower Exception Level using AArch64 */
  // Synchronous
  .align 7
  b el0_sync_a64
  // IRQ
  .align 7
  b el0_irq_a64
  // FIQ
  .align 7
  b el0_fiq_a64
  // SError
  .align 7
  b el0_error_a64

  /* Lower Exception Level using AArch32 */
  // Synchronous
  .align 7
  b el0_sync_a32
  // IRQ
  .align 7
  b el0_irq_a32
  // FIQ
  .align 7
  b el0_fiq_a32
  // SError
  .align 7
  b el0_error_a32

/* For now exception handlers just print a message and halts */

/* EL1 with SP_EL0 handlers */
el1_sync_sp0:
  // Print exception message
  adrp x0, str_el1_sync_sp0
  add x0, x0, :lo12:str_el1_sync_sp0
  bl uart_send_string
  b exception_hang

el1_irq_sp0:
  adrp x0, str_el1_irq_sp0
  add x0, x0, :lo12:str_el1_irq_sp0
  bl uart_send_string
  b exception_hang

el1_fiq_sp0:
  adrp x0, str_el1_fiq_sp0
  add x0, x0, :lo12:str_el1_fiq_sp0
  bl uart_send_string
  b exception_hang

el1_error_sp0:
  adrp x0, str_el1_error_sp0
  add x0, x0, :lo12:str_el1_error_sp0
  bl uart_send_string
  b exception_hang

/* EL1 with SP_ELx handlers */
el1_sync_spx:
  adrp x0, str_el1_sync_spx
  add x0, x0, :lo12:str_el1_sync_spx
  bl uart_send_string
  b exception_hang

el1_irq_spx:
  adrp x0, str_el1_irq_spx
  add x0, x0, :lo12:str_el1_irq_spx
  bl uart_send_string
  b exception_hang

el1_fiq_spx:
  adrp x0, str_el1_fiq_spx
  add x0, x0, :lo12:str_el1_fiq_spx
  bl uart_send_string
  b exception_hang

el1_error_spx:
  adrp x0, str_el1_error_spx
  add x0, x0, :lo12:str_el1_error_spx
  bl uart_send_string
  b exception_hang

/* EL0 AArch64 handlers */
el0_sync_a64:
  adrp x0, str_el0_sync_a64
  add x0, x0, :lo12:str_el0_sync_a64
  bl uart_send_string
  b exception_hang

el0_irq_a64:
  adrp x0, str_el0_irq_a64
  add x0, x0, :lo12:str_el0_irq_a64
  bl uart_send_string
  b exception_hang

el0_fiq_a64:
  adrp x0, str_el0_fiq_a64
  add x0, x0, :lo12:str_el0_fiq_a64
  bl uart_send_string
  b exception_hang

el0_error_a64:
  adrp x0, str_el0_error_a64
  add x0, x0, :lo12:str_el0_error_a64
  bl uart_send_string
  b exception_hang

/* EL0 AArch32 handlers - these should never be called */
el0_sync_a32:
el0_irq_a32:
el0_fiq_a32:
el0_error_a32:
  adrp x0, str_el0_aarch32
  add x0, x0, :lo12:str_el0_aarch32
  bl uart_send_string
  b exception_hang

exception_hang:
  adrp x0, str_exception_hang
  add x0, x0, :lo12:str_exception_hang
  bl uart_send_string
  // Infinite loop
1:
  wfe
  b 1b

.section ".rodata"
str_el1_sync_sp0:
  .asciz "EXCEPTION: EL1 Synchronous (SP_EL0)\n"
str_el1_irq_sp0:
  .asciz "EXCEPTION: EL1 IRQ (SP_EL0)\n"
str_el1_fiq_sp0:
  .asciz "EXCEPTION: EL1 FIQ (SP_EL0)\n"
str_el1_error_sp0:
  .asciz "EXCEPTION: EL1 SError (SP_EL0)\n"

str_el1_sync_spx:
  .asciz "EXCEPTION: EL1 Synchronous (SP_ELx)\n"
str_el1_irq_spx:
  .asciz "EXCEPTION: EL1 IRQ (SP_ELx)\n"
str_el1_fiq_spx:
  .asciz "EXCEPTION: EL1 FIQ (SP_ELx)\n"
str_el1_error_spx:
  .asciz "EXCEPTION: EL1 SError (SP_ELx)\n"

str_el0_sync_a64:
  .asciz "EXCEPTION: EL0/A64 Synchronous\n"
str_el0_irq_a64:
  .asciz "EXCEPTION: EL0/A64 IRQ\n"
str_el0_fiq_a64:
  .asciz "EXCEPTION: EL0/A64 FIQ\n"
str_el0_error_a64:
  .asciz "EXCEPTION: EL0/A64 SError\n"

str_el0_aarch32:
  .asciz "EXCEPTION: EL0/A32 Exception (unexpected)\n"

str_exception_hang:
  .asciz "System halted due to exception\n"

/*
 * boot.S - Second Stage bootloader for AArch64
 *
 * This file contains the second stage bootloader that takes over from start.
 * It performs basic hardware initialization and prepares to jump to the C kernel.
 *
 * Author: Fedi Nabli
 * Date: 26 Feb 2025
 * Last Modified: 26 Feb 2025
 */

.section ".text.boot.stage2"

.global boot_second_stage

boot_second_stage:
  // Output boot message
  adrp x0, str_stage2
  add x0, x0, :lo12:str_stage2
  bl uart_send_string

  // Setup minimal C environment

  // Configure system registers
  mrs x0, sctlr_el1 // System Control Register
  bic x0, x0, #(1 << 0) // Clear M bit (MMU) - ensure it's disabled initially
  bic x0, x0, #(1 << 1) // Clear A bit (alignment check)
  orr x0, x0, #(1 << 12) // Enable I-cache
  orr x0, x0, #(1 << 2) // Enable D-cache
  msr sctlr_el1, x0
  isb // Ensure changes take effect

  // Setup full vector table
  adrp x0, vector_table
  add x0, x0, :lo12:vector_table
  msr vbar_el1, x0
  isb

  // Print vector table initialized message
  adrp x0, str_vectors
  add x0, x0, :lo12:str_vectors
  bl uart_send_string

  // Initialize boot information structure
  bl init_boot_info

  // Print jumping to kernel message
  adrp x0, str_jumping_kernel
  add x0, x0, :lo12:str_jumping_kernel
  bl uart_send_string

  // Load boot info pointer as first parameter to the kernel
  adrp x0, boot_info
  add x0, x0, :lo12:boot_info

  // Zero out registers for clean kernel entry (preserving x0)
  mov x1, #0
  mov x2, #0
  mov x3, #0
  mov x4, #0
  mov x5, #0
  mov x6, #0
  mov x7, #0
  mov x8, #0
  mov x9, #0
  mov x10, #0
  mov x11, #0
  mov x12, #0
  mov x13, #0
  mov x14, #0
  mov x15, #0
  mov x16, #0
  mov x17, #0
  mov x18, #0
  mov x19, #0
  mov x20, #0
  mov x21, #0
  mov x22, #0
  mov x23, #0
  mov x24, #0
  mov x25, #0
  mov x26, #0
  mov x27, #0
  mov x28, #0
  mov x29, #0

  // Jump to kernel entry point (keeping x0 as parameter)
  adrp x1, kernel_main
  add x1, x1, :lo12:kernel_main
  mov x30, #0 // Zero Link Register
  br x1

// If kernel returns or no kernel is found, handle error
boot_error:
  adrp x0, str_boot_error
  add x0, x0, :lo12:str_boot_error
  bl uart_send_string
  b boot_halt

boot_halt:
  wfe
  b boot_halt

// Initialize boot information structure
init_boot_info:
  stp x29, x30, [sp, #-16]!
  mov x29, sp

  // Get boot_info structure address
  adrp x0, boot_info
  add x0, x0, :lo12:boot_info

  // Initialize magic number
  movz x1, #0x4F54
  movk x1, #0x424F, lsl #16 // BOOT in hexadecimal
  str x1, [x0]

  // Store Arm architecture info
  mov x1, #0x0
  str x1, [x0, #8] // ARMv8-A

  // Store memory information
  mov x1, #0x40000000 // 1 GB RAM size (must be later dynamic)
  str x1, [x0, #16]

  ldp x29, x30, [sp], #16
  ret

.section ".rodata"
str_jumping_kernel:
  .asciz "Jumping to kernel, bootloader done...\n"
str_boot_error:
  .asciz "ERROR: Boot failure - kernel returned or not found\n"
str_stage2:
  .asciz "Second-stage bootloader started\n"
str_vectors:
  .asciz "Vector table initialized\n"

/* BOOT Info structure */
.section ".data"
.align 8
boot_info:
  .skip 64 // 64 bytes for boot information structure

/* Define a weak symbol for kernel main to handle missing kernel */
.section ".text"
.weak kernel_main
kernel_main:
  adrp x0, str_no_kernel
  add x0, x0, :lo12:str_no_kernel
  bl uart_send_string
  b boot_error

.section ".rodata"
str_no_kernel:
  .asciz "ERROR: No kernel entry point found!\n"

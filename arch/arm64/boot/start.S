/*
 * start.S - First Stage bootloader for AArch64
 *
 * This is the entry point of the kernel when loaded by Qemu.
 * It performs the bare minimum setup to transition to the
 * second-stage bootloader.
 *
 * Author: Fedi Nabli
 * Date: 25 Feb 2025
 * Last Modified: 26 Feb 2025
 */

.section ".text.boot"

.global _start

_start:
  // Check which CPU we are on, if not primary we halt (we should be on CPU0)
  mrs x0, mpidr_el1
  and x0, x0, #0xFF // Extract Aff0 field (Core ID)
  cbz x0, primary_cpu // If it's core 0, continue

secondary_halt:
  wfe
  b secondary_halt

primary_cpu:
  // Disable all interrupts
  msr daifset, #0xF // Set all interrupt mask bits in PSTATE

  // Initialize the stack pointer (use end of the stack defined in linked script)
  adrp x0, __stack_top
  add x0, x0, :lo12:__stack_top
  mov sp, x0

  // Initialize UART for debug output
  bl uart_init

  // Print first boot message
  adrp x0, str_stage1
  add x0, x0, :lo12:str_stage1
  bl uart_send_string

  // Determine current exception level
  mrs x0, CurrentEL
  lsr x0, x0, #2

  // Branch based on current Exception Level
  cmp x0, #3
  beq el3_setup
  cmp x0, #2
  beq el2_setup
  cmp x0, #1
  beq el1_setup

  // Should never reach here - hang if we do
  adrp x0, str_unknown_el
  add x0, x0, :lo12:str_unknown_el
  bl uart_send_string
  b hang

el3_setup:
  adrp x0, str_el3
  add x0, x0, :lo12:str_el3
  bl uart_send_string

  // If we are in EL3, prepare to drop to EL2
  ldr x0, =0x5B1 // EL1 and EL2 are AArch64
  msr scr_el3, x0

  ldr x0, =0x1C9 // FPEN enabled, EL2 access to timers
  msr cptr_el3, x0

  mov x0, #0x80000000 // Vector table base address
  msr vbar_el3, x0

  // Prepare for EL2 entry and drop to EL2
  ldr x0, =el2_entry
  msr elr_el3, x0

  mov x0, #0x3C5 // Use SP_ELx, disable interrupts
  msr spsr_el3, x0

  eret // Drop to EL2

el2_entry:
  // Entry point when dropping from EL3 to EL2
  b el2_setup

el2_setup:
  adrp x0, str_el2
  add x0, x0, :lo12:str_el2
  bl uart_send_string

  // If we are in EL2, prepare to drop to EL1
  ldr x0, =0x80000000 // Set EL2 vector base
  msr vbar_el2, x0

  // Initialize Hypervisor Configuration Register
  mov x0, #(1 << 31) // RW=1 (EL1 is AArch64)
  msr hcr_el2, x0

  // Prepare for EL1 entry and drop to EL1
  ldr x0, =el1_entry
  msr elr_el2, x0

  mov x0, #0x3C5 // Use SP_ELx, disable interrupts
  msr spsr_el2, x0

  eret // Drop to EL1

el1_entry:
  // Entry point when dropping from EL2 to EL1
  b el1_setup

el1_setup:
  adrp x0, str_el1
  add x0, x0, :lo12:str_el1
  bl uart_send_string

  // We are now in EL1, the level the kernel will run on

  // Initialize early vector table
  adrp x0, vector_table // Load address of vector table
  add x0, x0, :lo12:vector_table
  msr vbar_el1, x0 // set vector base address register
  isb // Instruction synchronization barrier

  // Clear BSS section
  adrp x0, __bss_start // Start address of BSS
  add x0, x0, :lo12:__bss_start
  adrp x1, __bss_end // End address of BSS
  add x1, x1, :lo12:__bss_end

clear_bss:
  cmp x0, x1 // Check if we've reached the end
  beq bss_cleared // If so, BSS is cleared
  str xzr, [x0], #8 // Clear 8 bytes and increment
  b clear_bss

bss_cleared:
  // Jump to second stage bootloader message
  adrp x0, str_jumping
  add x0, x0, :lo12:str_jumping
  bl uart_send_string

  // Jump to second stage bootloader
  adrp x4, boot_second_stage
  add x4, x4, :lo12:boot_second_stage
  br x4

memory_error:
  // Handle memory error - display error on debug port
  adrp x0, str_mem_error
  add x0, x0, :lo12:str_mem_error
  bl uart_send_string
  b hang

hang:
  // Display hang message
  adrp x0, str_hang
  add x0, x0, :lo12:str_hang
  bl uart_send_string

  // Infinite loop if something goes wrong
  wfe
  b hang

.section ".rodata"
str_unknown_el:
  .asciz "ERROR: Unknown Exception Level\n"
str_el3:
  .asciz "Running at EL3, preparing to drop to EL2\n"
str_el2:
  .asciz "Running at EL2, preparing to drop to EL1\n"
str_el1:
  .asciz "Running at EL1, continuing boot\n"
str_mem_error:
  .asciz "ERROR: Memory validation failed\n"
str_jumping:
  .asciz "Jumping to second-stage bootloader\n"
str_hang:
  .asciz "System halted\n"
str_stage1:
  .asciz "First-stage bootloader started\n"

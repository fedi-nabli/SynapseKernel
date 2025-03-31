/*
 * context_switch.S - Assembly routines for AArch64 context switching
 *
 * Author: Fedi Nabli
 * Date: 31 Mar 2025
 * Last Modified: 31 Mar 2025
 */

.section .text

.global task_save_context
.global task_restore_context

/**
 * Save current task context
 * void task_save_context();
 */
task_save_context:
  // Get current task structure from global variable
  adrp x9, current_task
  add x9, x9, :lo12:current_task
  ldr x9, [x9]

  // If no current task, just return
  cbz x9, 1f

  // Calculate offset registers structure (8 byte in struct task)
  add x9, x9, #8

  // Save general purpose registers x0-x30
  stp x0, x1, [x9, #0]
  stp x2, x3, [x9, #16]
  stp x4, x5, [x9, #32]
  stp x6, x7, [x9, #48]
  stp x8, x9, [x9, #64]
  mov x11, x9 // Move x9 in x11 since we're using x9 as a pointer
  stp x10, x11, [x9, #80]
  stp x12, x13, [x9, #96]
  stp x14, x15, [x9, #112]
  stp x16, x17, [x9, #128]
  stp x18, x19, [x9, #144]
  stp x20, x21, [x9, #160]
  stp x22, x23, [x9, #176]
  stp x24, x25, [x9, #192]
  stp x26, x27, [x9, #208]
  stp x28, x29, [x9, #224]
  str x30, [x9, #240] // Store x30 (link register)

  // Save stack pointer
  mov x0, sp
  str x0, [x9, #248]

  // Save PC (return address) - use LR
  str x30, [x9, #256] // Store PC at offset 256

  // Store SPSR_EL1 and ELR_EL1
  mrs x0, spsr_el1
  str x0, [x9, #264]
  mrs x0, elr_el1
  str x0, [x9, #272]
1: // Return
  ret

/**
 * Restore task context and switch to it
 * void task_restore_context(struct task* task);
 * task is passed in x0
 */
task_restore_context:
  // If task pointer is NULL, just return
  cbz x0, 2f

  // Calculate offset registers structure (8 byte in struct task)
  add x9, x0, #8

  // Save pointer to task structure in global
  adrp x0, current_task
  add x0, x0, :lo12:current_task
  str x9, [x0]

  // Load SPSR_EL1 and ELR_EL1
  ldr x0, [x9, #264] // Load SPSR_EL1 from offset 264
  msr spsr_el1, x0
  ldr x0, [x9, #272] // load ELR_EL1 from offset 272
  msr elr_el1, x0

  // Load stack pointer (sp)
  ldr x0, [x9, #248] // Load SP from offset 248
  mov sp, x0

  // Load general purpose registers x2-x30 (leaving x0,x1 for last)
  ldp x2, x3, [x9, #16]
  ldp x4, x5, [x9, #32]
  ldp x6, x7, [x9, #48]
  ldp x8, x10, [x9, #64] // Skip x9 for now as we're using it
  ldp x12, x13, [x9, #96]
  ldp x14, x15, [x9, #112]
  ldp x16, x17, [x9, #128]
  ldp x18, x19, [x9, #144]
  ldp x20, x21, [x9, #160]
  ldp x22, x23, [x9, #176]
  ldp x24, x25, [x9, #192]
  ldp x26, x27, [x9, #208]
  ldp x28, x29, [x9, #224]
  ldr x30, [x9, #240] // Load x30 (Link Register)

  // Now load x0, x1 and x9
  ldp x0, x1, [x9, #0]
  ldr x9, [x9, #72]

  // Check if we're returning to EL1 or EL0
  mrs x10, spsr_el1
  and x10, x10, #0xF // Extract M[3:0] bits
  cmp x10, #0 // 0 = EL0t (EL0 with SP0)
  beq switch_to_el0

  // EL1 return - just return to address in ELR_EL1
  eret

switch_to_el0:
  // Return to EL0
  eret

2: // Return if Task is NULL
  ret

/* Global variables */
.section .data
.extern current_task
current_task:
  .quad 0 // Pointer to current task (64-bit)

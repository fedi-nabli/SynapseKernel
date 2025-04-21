/*
 * context_switch.S - Assembly routines for AArch64 context switching
 *
 * Author: Fedi Nabli
 * Date: 31 Mar 2025
 * Last Modified: 9 Apr 2025
 */

.section .text

.global task_save_context
.global task_restore_context
.global svc_handler
.global irq_handler_entry

/* Constants for struct task_registers offsets */
.equ TASK_REGS_OFFSET, 8
.equ REGS_X19_OFFSET,  152   /* 19 * 8 */
.equ REGS_X21_OFFSET,  168   /* 21 * 8 */
.equ REGS_X23_OFFSET,  184   /* 23 * 8 */
.equ REGS_X25_OFFSET,  200   /* 25 * 8 */
.equ REGS_X27_OFFSET,  216   /* 27 * 8 */
.equ REGS_X29_OFFSET,  232   /* 29 * 8 */
.equ REGS_SP_OFFSET,   248   /* 31 * 8 */
.equ REGS_PC_OFFSET,   256   /* 32 * 8 */
.equ REGS_SPSR_OFFSET, 264   /* 33 * 8 */
.equ REGS_ELR_OFFSET,  272   /* 34 * 8 */

/**
 * Save current task context
 * void task_save_context();
 */
.type task_save_context, %function
task_save_context:
  // Load current_task
  adrp x9, current_task
  add  x9, x9, :lo12:current_task
  ldr  x9, [x9]
  cbz  x9, .Lsave_done

  // Pointer to task->registers
  add  x10, x9, #TASK_REGS_OFFSET

  // Store x19-x30
  stp  x19, x20, [x10, #REGS_X19_OFFSET]
  stp  x21, x22, [x10, #REGS_X21_OFFSET]
  stp  x23, x24, [x10, #REGS_X23_OFFSET]
  stp  x25, x26, [x10, #REGS_X25_OFFSET]
  stp  x27, x28, [x10, #REGS_X27_OFFSET]
  stp  x29, x30, [x10, #REGS_X29_OFFSET]

  // Save SP
  mov  x0, sp
  str  x0, [x10, #REGS_SP_OFFSET]

  // Save PC (LR)
  mov  x0, lr
  str  x0, [x10, #REGS_PC_OFFSET]

.Lsave_done:
  ret

/*
 * void task_restore_context(struct task* task);
 * Restore SPSR, ELR, SP, callee-saved regs, then ERET
 */
.type task_restore_context, %function
task_restore_context:
  cbz     x0, .Lnull_task

  /* pointer to task->registers */
  add     x10, x0, #TASK_REGS_OFFSET

  /* ---- 1. restore SPSR & ELR ---------------------------------- */
  ldr     x1, [x10, #REGS_SPSR_OFFSET]     /* x1 = SPSR_EL1  */
  msr     spsr_el1, x1
  ldr     x2, [x10, #REGS_ELR_OFFSET]      /* x2 = ELR_EL1   */
  msr     elr_el1, x2

  /* ---- 2. set stacks correctly -------------------------------- */
  ldr     x2, [x10, #REGS_SP_OFFSET]       /* x2 = task SP   */

  and     x3, x1, #0xF                     /* target EL */
  cbnz    x3, 1f                           /* !=0 → EL1h task */

  /* ---- returning to EL0t ---- */
  msr     sp_el0, x2                       /* give user its stack */
  b       2f                               /* keep SP_EL1 unchanged */

1:  /* ---- returning to EL1h (kernel thread) ---- */
  mov     sp,  x2                          /* switch GP stack   */
2:
  dsb     sy
  isb

  /* ---- 3. restore callee‑saved regs --------------------------- */
  ldp     x19, x20, [x10, #REGS_X19_OFFSET]
  ldp     x21, x22, [x10, #REGS_X21_OFFSET]
  ldp     x23, x24, [x10, #REGS_X23_OFFSET]
  ldp     x25, x26, [x10, #REGS_X25_OFFSET]
  ldp     x27, x28, [x10, #REGS_X27_OFFSET]
  ldp     x29, x30, [x10, #REGS_X29_OFFSET]

  /* ---- 4. go run the task ------------------------------------ */
  eret

.Lnull_task:
  ret

/**
 * SVC handler - called from el0_sync_a64 vector 
 * This handles system calls from EL0
 */
.type svc_handler,%function
svc_handler:
  /* ---- make room for struct interrupt_frame (272 bytes) ---- */
  sub     sp, sp, #0x110              /* 0x110 = 272 */

  /* ---- save general‑purpose registers x0–x30 -------------- */
  stp     x0,  x1,  [sp,  #0]
  stp     x2,  x3,  [sp, #16]
  stp     x4,  x5,  [sp, #32]
  stp     x6,  x7,  [sp, #48]
  stp     x8,  x9,  [sp, #64]
  stp     x10, x11, [sp, #80]
  stp     x12, x13, [sp, #96]
  stp     x14, x15, [sp,#112]
  stp     x16, x17, [sp,#128]
  stp     x18, x19, [sp,#144]
  stp     x20, x21, [sp,#160]
  stp     x22, x23, [sp,#176]
  stp     x24, x25, [sp,#192]
  stp     x26, x27, [sp,#208]
  stp     x28, x29, [sp,#224]
  str     x30,       [sp,#240]       /* save LR */

  /* ---- save EL0‑stack pointer and return state ----------- */
  mrs     x0,  sp_el0                 /* user SP_EL0 */
  str     x0, [sp,#248]
  mrs     x20, elr_el1                /* saved ELR_EL1 */
  str     x20, [sp,#256]
  mrs     x21, spsr_el1               /* saved SPSR_EL1 */
  str     x21, [sp,#264]

  /* ---- call into C dispatcher --------------------------- */
  mov     x1, sp                       /* 2nd arg = frame* */
  bl      svc_c_handler                /* returns in x0  */
  str     x0, [sp]                     /* frame->x0 = retval */

  /* ---- restore ELR_EL1 & SPSR_EL1 ----------------------- */
  msr     elr_el1, x20
  msr     spsr_el1, x21

  /* ---- restore registers x0–x30 ------------------------- */
  ldp     x0,  x1,  [sp,  #0]
  ldp     x2,  x3,  [sp, #16]
  ldp     x4,  x5,  [sp, #32]
  ldp     x6,  x7,  [sp, #48]
  ldp     x8,  x9,  [sp, #64]
  ldp     x10, x11, [sp, #80]
  ldp     x12, x13, [sp, #96]
  ldp     x14, x15, [sp,#112]
  ldp     x16, x17, [sp,#128]
  ldp     x18, x19, [sp,#144]
  ldp     x20, x21, [sp,#160]
  ldp     x22, x23, [sp,#176]
  ldp     x24, x25, [sp,#192]
  ldp     x26, x27, [sp,#208]
  ldp     x28, x29, [sp,#224]
  ldr     x30,       [sp,#240]

  /* ---- pop the frame & return to EL0 -------------------- */
  add     sp, sp, #0x110
  eret

.size   svc_handler, .-svc_handler

/**
 * IRQ handler - called from el1_irq_spx and el0_irq_a64 vectors
 * This handles interrupts for both EL0 and EL1
 */
irq_handler_entry:
  // Create stack frame 
  sub sp, sp, #544

  // Save all registers
  stp x0, x1, [sp, #0]
  stp x2, x3, [sp, #16]
  stp x4, x5, [sp, #32]
  stp x6, x7, [sp, #48]
  stp x8, x9, [sp, #64]
  stp x10, x11, [sp, #80]
  stp x12, x13, [sp, #96]
  stp x14, x15, [sp, #112]
  stp x16, x17, [sp, #128]
  stp x18, x19, [sp, #144]
  stp x20, x21, [sp, #160]
  stp x22, x23, [sp, #176]
  stp x24, x25, [sp, #192]
  stp x26, x27, [sp, #208]
  stp x28, x29, [sp, #224]
  str x30, [sp, #240]

  // Save SP_EL0 if coming from EL0
  mrs x0, spsr_el1
  and x0, x0, #0xF // Extract M[3:0] bits
  cmp x0, #0 // 0 = EL0t
  bne 1f
  mrs x0, sp_el0
  b 2f
1:
  mov x0, sp // Use current SP if not from EL0
2:
  str x0, [sp, #248]

  // Save ELR_EL1 and SPSR_EL1
  mrs x0, elr_el1
  str x0, [sp, #256]
  mrs x0, spsr_el1
  str x0, [sp, #264]

  // Call C handler with frame
  mov x0, sp
  bl irq_handler

  // restore registers
  ldp x0, x1, [sp, #0]
  ldp x2, x3, [sp, #16]
  ldp x4, x5, [sp, #32]
  ldp x6, x7, [sp, #48]
  ldp x8, x9, [sp, #64]
  ldp x10, x11, [sp, #80]
  ldp x12, x13, [sp, #96]
  ldp x14, x15, [sp, #112]
  ldp x16, x17, [sp, #128]
  ldp x18, x19, [sp, #144]
  ldp x20, x21, [sp, #160]
  ldp x22, x23, [sp, #176]
  ldp x24, x25, [sp, #192]
  ldp x26, x27, [sp, #208]
  ldp x28, x29, [sp, #224]
  ldr x30, [sp, #240]

  // Clean up stack
  add sp, sp, #544

  // Return from exception
  eret

.extern current_task
current_task:
  .quad 0 // Pointer to current task (64-bit)

.section .data
debug_str1:
  .asciz "task_restore_context: Starting\n"
debug_str2:
  .asciz "task_restore_context: Set current_task\n"
debug_str3:
  .asciz "task_restore_context: Set SPSR_EL1\n"
debug_str4:
  .asciz "task_restore_context: Set ELR_EL1\n"
debug_str5:
  .asciz "task_restore_context: About to ERET\n"
debug_str_null:
  .asciz "task_restore_context: ERROR - NULL task pointer\n"

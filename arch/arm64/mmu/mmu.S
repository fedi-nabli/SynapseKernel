/*
 * mmsu.S - Memory Management Unit for AArch64 ArmV8
 *
 * This file contains necessary functions to control the MMU
 *
 * Author: Fedi Nabli
 * Date: 9 Mar 2025
 * Last Modified: 9 Mar 2025
 */

.section ".text"

.global read_sctlr_el1
.global write_sctlr_el1
.global read_tcr_el1
.global write_tcr_el1
.global read_mair_el1
.global write_mair_el1
.global read_ttbr0_el1
.global write_ttbr0_el1
.global read_ttbr1_el1
.global write_ttbr1_el1
.global invalidate_tlb
.global invalidate_tlb_entry
.global dsb_sy
.global isb_sy

.global enable_mmu_assembly

// function to read SCTLR_EL1 (System Control Register)
// uint64_t read_sctlr_el1(void);
read_sctlr_el1:
  mrs x0, sctlr_el1
  ret

// function to write SCTLR_EL1 (System Control Register)
// void write_sctlr_el1(uint64_t value);
write_sctlr_el1:
  msr sctlr_el1, x0
  // Ensure the write completes before 
  dsb sy
  isb
  ret

// function to read TCR_EL1 (Translation Control Register)
// unint64_t read_tcr_el1(void);
read_tcr_el1:
  mrs x0, tcr_el1
  ret

// function to write TCR_EL1 (Translation Control Register)
// void write_tcr_el1(uint64_t value);
write_tcr_el1:
  msr tcr_el1, x0
  dsb sy
  isb
  ret

// function to read MAIR_EL1 (Memory Attribute Indirection Register)
// uint64_t read_mair_el1(void);
read_mair_el1:
  mrs x0, mair_el1
  ret

// function to write MAIR_EL1 (Memory Attribute Indirection Register)
// void write_mair_el1(uint64_t value);
write_mair_el1:
  msr mair_el1, x0
  dsb sy
  isb
  ret

// function to read TTBR0_EL1 (Translation Table Base Register 0)
// uint64_t read_ttbr0_el1(void);
read_ttbr0_el1:
  mrs x0, ttbr0_el1
  ret

// function to write TTBR0_EL1 (Translation Table Base Register 0)
// void write_ttbr0_el1(uint64_t value);
write_ttbr0_el1:
  msr ttbr0_el1, x0
  dsb sy
  isb
  ret

// function to read TTBR1_EL1 (Translation Table Base Register 1)
// uint64_t read_ttbr1_el1(void);
read_ttbr1_el1:
  mrs x0, ttbr1_el1
  ret

// function to write TTBR1_EL1 (Translation Table Base Register 1)
// void write_ttbr1_el1(uint64_t value);
write_ttbr1_el1:
  msr ttbr1_el1, x0
  dsb sy
  isb
  ret

// function to invalidate all TLB (Translation Lookaside Buffer) entries
// void invalidate_tlb(void);
invalidate_tlb:
  // Invalidate all TLB entries at current EL
  tlbi vmalle1
  dsb sy
  isb
  ret

// function to invalidate a specific TLB entry
// void invalidate_tlb_entry(uint64_t vaddr);
invalidate_tlb_entry:
  // Invalidate TLB entry fir the given virtual address
  tlbi vaae1, x0
  dsb sy
  isb
  ret

// Data Synchronization Barrier (System)
// void dsb_sy(void);
dsb_sy:
  dsb sy
  ret

// Instruction Synchronization Barrier
// void isb_sy(void)
isb_sy:
  isb
  ret

// Function to safely enable the MMU
// void enable_mmu_assembly(uint64_t ttbr0, uint64_t ttbr1);
enable_mmu_assembly:
  // Save registers
  stp x29, x30, [sp, #-16]!
  mov x29, sp
  
  // Save TTBR values
  mov x2, x0      // ttbr0
  mov x3, x1      // ttbr1

  // Ensure TLBs are clear
  tlbi vmalle1is
  dsb ish
  isb

  // Get post-MMU address
  adr x4, mmu_enabled_return

  // Prepare SCTLR_EL1 value
  mrs x0, sctlr_el1
  mov x1, #1      // MMU enable
  orr x1, x1, #(1 << 2)    // C bit
  orr x1, x1, #(1 << 12)   // I bit
  orr x0, x0, x1 
  
  // Enable MMU
  dsb sy          // Ensure all prior changes complete
  msr sctlr_el1, x0
  isb             // Flush pipeline

  // Jump to virtual address
  br x4

mmu_enabled_return:
  // Verify MMU enabled
  mrs x0, sctlr_el1
  and x0, x0, #1
  cbnz x0, 1f    // Check if MMU is on

  // MMU failed to enable
  mov x0, #-1
  ret

1:  // MMU enabled successfully
  mov x0, #0
  // Restore registers
  ldp x29, x30, [sp], #16
  ret

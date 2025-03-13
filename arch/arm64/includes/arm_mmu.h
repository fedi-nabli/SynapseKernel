/*
 * arm_mmu.h - Memory Management Unit for AArch64 (ARMv8)
 *
 * This file defines bits and attributes for configuring the MMU
 * and exports functions for controlling the registers from assembly and C
 * 
 * Author: Fedi Nabli
 * Date: 10 Mar 2025
 * Last Modified: 13 Mar 2025
*/

#ifndef __AARCH64_MMU_H_
#define __AARCH64_MMU_H_

#include <synapse/types.h>

/* System Control Register (SCTLR_EL1) bits */
#define SCTLR_EL1_M   (1UL << 0) // MMU Enable
#define SCTLR_EL1_A   (1UL << 1) // Alignment check enable
#define SCTLR_EL1_C   (1UL << 2) // Data cache enable
#define SCTLR_EL1_SA  (1UL << 3) // Stack alignment check enable
#define SCTLR_EL1_SA0 (1UL << 4) // Stack elignment check enable for EL0
#define SCTLR_EL1_I   (1UL << 12) // Instruction cache enable
#define SCTLR_EL1_WXN (1UL << 19) // Write permission implies XN
#define SCTLR_EL1_EE  (1UL << 25) // Exception Endianess (0: little, 1: big)
#define SCTLR_EL1_UCI (1UL << 26) // Cache Instruction available at EL0

/* Translation Control Register (TCR_EL1) bits */
#define TCR_EL1_T0SZ_SHIFT  0
#define TCR_EL1_EPD0        (1UL << 7) // Translation Table walk disable for TTBR0_EL1
#define TCR_EL1_IRGN0_SHIFT 8
#define TCR_EL1_ORGN0_SHIFT 10
#define TCR_EL1_SH0_SHIFT   12
#define TCR_EL1_TG0_SHIFT   14
#define TCR_EL1_T1SZ_SHIFT  16
#define TCR_EL1_A1          (1UL << 22) // ASID size (0: 8bit, 1: 16bit)
#define TCR_EL1_EPD1        (1UL << 23) // Translation Table walk disable for TTBR1_EL1
#define TCR_EL1_IRGN1_SHIFT 24
#define TCR_EL1_ORGN1_SHIFT 26
#define TCR_EL1_SH1_SHIFT   28
#define TCR_El1_TG1_SHIFT   30
#define TCR_EL1_IPS_SHIFT   32
#define TCR_EL1_AS          (1UL << 36) // ASID size (0: 8bit, 1: 16bit)
#define TCR_EL1_TBI0        (1UL << 37) // Top Byte Ignored for TTBR0_EL1
#define TCR_EL1_TBI1        (1UL << 38) // Top Byte Ignored for TTBR1_EL1

/* Values for TCR_EL1 fields */
#define TCR_IRGN_WBWA   0x01 // Inner Write-Back Read-Allocate Write-Allocate Cacheable
#define TCR_ORGN_WBWA   0x01 // Outer Write-Back Read-Allocate Write-Allocate Cacheable
#define TCR_SH_INNER    0x03 // Inner Shearable
#define TCR_TG0_4K      0x00 // 4KB granule for TTBR0
#define TCR_TG1_4K      0x02 // 4KB granule for TTBR1
#define TCR_IPS_40BITS  0x02 // 40-bit physical address space (typical for i.MX8)

/* Memory Attribute Indirection Register (MAIR_EL1) attributes */
#define MAIR_ATTR_SHIFT(n)    ((n) << 3)
#define MAIR_ATTR_MASK(n)     (0xFFUL << MAIR_ATTR_SHIFT(n))
#define MAIR_ATTR(n, v)       (((uint64_t)(v)) << MAIR_ATTR_SHIFT(n))

/* Memory attribute values */
#define MAIR_DEVICE_nGnRnE    0x00 // Device memory non-Gathering, non-Recording, no Early write acknowledgement
#define MAIR_DEVICE_nGnRE     0x04 // Device memory non-Gathering, non-Recording, Early write acknowledgement
#define MAIR_DEVICE_GRE       0x0C // Device memory Gathering, Recording, Early write acknowledgement
#define MAIR_NORMAL_NC        0x44 // Normal memory, Outer and Inner Non-Cacheable
#define MAIR_NORMAL_WT        0xBB // Normal memory, Outer and Inner Write-Through, Read-Allocate
#define MAIR_NORMAL_WB        0xFF // Normal memory, Outer and Inner Write-Back, Read_Allocate, Write-Allocate

/*  */
#define MEMORY_ATTR_DEVICE_nGnRnE   0 // Device memory non-Gathering, non-Recording, no Early write acknowledgement
#define MEMORY_ATTR_DEVICE_nGnRE    1 // Device memory non-Gathering, non-Recording, Early write acknowledgement
#define MEMORY_ATTR_DEVICE_GRE      2 // Device memory Gathering, Recording, Early write acknowledgement
#define MEMORY_ATTR_NORMAL_NC       3 // Normal memory, Outer and Inner Non-Cacheable
#define MEMORY_ATTR_NORMAL_WT       4 // Normal memory, Outer and Inner Write-Through, Read-Allocate
#define MEMORY_ATTR_NORMAL_WB       5 // Normal memory, Outer and Inner Write-Back, Read_Allocate, Write-Allocate

/* Translation table entry types */
#define PTE_TYPE_FAULT  0 // Invalid entry
#define PTE_TYPE_BLOCK  1 // Block entry (only valid at leverl 1 and 2)
#define PTE_TYPE_TABLE  3 // Table entry (not valid at level 3)
#define PTE_TYPE_PAGE   3 // Table entry (only valid at level 3)

/* Translation table entry bits */
#define PTE_TYPE_MASK         0x03 // Entry type masl
#define PTE_TABLE_ADDR_MASK   0xFFFFFFFFF000 // Physical address mask for table entries
#define PTE_BLOCK_ADDR_MASK   0xFFFFFFFFF000 // Physical address mask for block entries

/* Page attribute bits (for both block and page entries) */
#define PTE_ATTR_AF         (1UL << 10) // Access flag
#define PTE_ATTR_SH_INNER   (3UL << 8) // Inner Shearable
#define PTE_ATTR_SH_OUTER   (2UL << 8) // Outer Shearable
#define PTE_ATTR_SH_NON     (0UL << 8) // Non-Shearable
#define PTE_ATTR_AP_RW_EL1  (0UL << 6) // RW at EL1, no access at EL0
#define PTE_ATTR_AP_RW_ALL  (1UL << 6) // RW at all ELs
#define PTE_ATTR_AP_RO_EL1  (2UL << 6) // RO at EL1, no access at EL0
#define PTE_ATTR_AP_RO_ALL  (3UL << 6) // RO at all ELs
#define PTE_ATTR_UXN        (1UL << 54) // Unpriviliged Execute Never
#define PTE_ATTR_PXN        (1UL << 53) // Priviliged Execute Never
#define PTE_ATTR_ATTR_INDX(n)  ((uint64_t)(n) << 2) // Memory Attribute Index

uint64_t read_sctlr_el1();
void write_sctlr_el1(uint64_t value);
uint64_t read_tcr_el1();
void write_tcr_el1(uint64_t value);
uint64_t read_mair_el1();
void write_mair_el1(uint64_t value);
uint64_t read_ttbr0_el1();
void write_ttbr0_el1(uint64_t value);
uint64_t read_ttbr1_el1();
void write_ttbr1_el1(uint64_t value);
void invalidate_tlb();
void invalidate_tlb_entry(uint64_t vaddr);
void dsb_sy();
void isb_sy();

/**
 * @brief Configure System Control Register (SCTLR_EL1)
 * 
 * This function configures the SCTLR_EL1 register with appropriate settings
 * for the kernel. The MMU will not be enabled here; that will happen in mmu_enable().
 * 
 * @author Fedi Nabli
 * @date 13 Mar 2025
 */
void configure_sctlr_el1();

/**
 * @brief Configure Translation Control Register (TCR_EL1)
 * 
 * This function sets up the TCR_EL1 register to define characteristics
 * of the memory translation system.
 * 
 * @author Fedi Nabli
 * @date 13 Mar 2025
 */
void configure_tcr_el1();

/**
 * @brief Configure Memory Attribute Indirection Register (MAIR_EL1)
 * 
 * This function configures memory attributes for different types of memory.
 * It defines how memory is cached, buffered and shared.
 * 
 * @author Fedi Nabli
 * @date 13 Mar 2025
 */
void configure_mair_el1();

#endif
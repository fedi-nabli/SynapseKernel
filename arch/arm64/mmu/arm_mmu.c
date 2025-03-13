/*
 * arm_mmu.c - Memory Management Unit for AArch64 (ARMv8)
 *
 * This file defines functions for controlling the registers from C
 * and configures important registers.
 * 
 * Author: Fedi Nabli
 * Date: 10 Mar 2025
 * Last Modified: 13 Mar 2025
*/

#include <arm_mmu.h>
#include <synapse/types.h>

/**
 * @brief Configure System Control Register (SCTLR_EL1)
 * 
 * This function configures the SCTLR_EL1 register with appropriate settings
 * for the kernel. The MMU will not be enabled here; that will happen in mmu_enable().
 * 
 * @author Fedi Nabli
 * @date 13 Mar 2025
 */
void configure_sctlr_el1()
{
  uint64_t sctlr = read_sctlr_el1();

  // First, clear bits we want to modify explicitly
  sctlr &= ~(SCTLR_EL1_M | SCTLR_EL1_A | SCTLR_EL1_C | SCTLR_EL1_I |
              SCTLR_EL1_WXN | SCTLR_EL1_SA | SCTLR_EL1_SA0);

  // Now we set the desired configuration
  // We're not enabling MMU (SCTLR_EL1_M) here; it will be enabled later
  sctlr |= SCTLR_EL1_C; // Enable data cache
  sctlr |= SCTLR_EL1_I; // Enable instruction cache
  sctlr |= SCTLR_EL1_SA; // Enable stack alignment check for EL1
  sctlr |= SCTLR_EL1_SA0; // Enable stack alignment check for EL0

  // Write the new value back to SCTLR_EL1
  write_sctlr_el1(sctlr);
}

/**
 * @brief Configure Translation Control Register (TCR_EL1)
 * 
 * This function sets up the TCR_EL1 register to define characteristics
 * of the memory translation system.
 * 
 * @author Fedi Nabli
 * @date 13 Mar 2025
 */
void configure_tcr_el1()
{
  uint64_t tcr = 0;

  // Configure address space size
  // T0SZ and T1SZ determine the size of the virtual address space
  // For a 48-bit VA space (common in ARMv8), use 64 - 48 = 16
  tcr |= (16UL << TCR_EL1_T0SZ_SHIFT); // TTBR0 covers 48-bit VA (user space)
  tcr |= (16UL << TCR_EL1_T1SZ_SHIFT); // TTBR1 covers 48-bit VA (kernel space)

  // Configure granule size
  tcr |= (TCR_TG0_4K << TCR_EL1_TG0_SHIFT); // 4KB page for TTBR0
  tcr |= (TCR_TG1_4K << TCR_El1_TG1_SHIFT); // 4KB page for TTBR1

  // Configure cacheability of translation table walks
  // Inner cacheability
  tcr |= (TCR_IRGN_WBWA << TCR_EL1_IRGN0_SHIFT); // TTBR0 walks are write-back, read/write allocate
  tcr |= (TCR_IRGN_WBWA << TCR_EL1_IRGN1_SHIFT); // TTBR1 walks are write-back, read/write allocate

  // Outer cacheability
  tcr |= (TCR_ORGN_WBWA << TCR_EL1_ORGN0_SHIFT); // TTBR0 walks are write-back, read/write allocate
  tcr |= (TCR_ORGN_WBWA << TCR_EL1_ORGN1_SHIFT); // TTBR1 walks are write-back, read/write allocate

  // Configure shearability of tranlation table walks
  tcr |= (TCR_SH_INNER << TCR_EL1_SH0_SHIFT); // TTBR0 walks are inner shearable
  tcr |= (TCR_SH_INNER << TCR_EL1_SH1_SHIFT); // TTBR1 walks are inner shearable

  // Configure physical address size (IPS)
  tcr |= ((uint64_t)TCR_IPS_40BITS << 32ULL);

  // Write to TCR_EL1 register
  write_tcr_el1(tcr);
}

/**
 * @brief Configure Memory Attribute Indirection Register (MAIR_EL1)
 * 
 * This function configures memory attributes for different types of memory.
 * It defines how memory is cached, buffered and shared.
 * 
 * @author Fedi Nabli
 * @date 13 Mar 2025
 */
void configure_mair_el1()
{
  uint64_t mair = 0;

  // Configure memory attributes for different kinds of memory
  mair |= MAIR_ATTR(MEMORY_ATTR_DEVICE_nGnRnE, MAIR_DEVICE_nGnRnE); // Device memory (stringly ordered, non gathering, no reordering)
  mair |= MAIR_ATTR(MEMORY_ATTR_DEVICE_nGnRE, MAIR_DEVICE_nGnRE); // Device memory (with early write back)
  mair |= MAIR_ATTR(MEMORY_ATTR_DEVICE_GRE, MAIR_DEVICE_GRE);
  mair |= MAIR_ATTR(MEMORY_ATTR_NORMAL_NC, MAIR_NORMAL_NC); // Normal memory, non-cacheable
  mair |= MAIR_ATTR(MEMORY_ATTR_NORMAL_WT, MAIR_NORMAL_WT); // Normal memory, write-through
  mair |= MAIR_ATTR(MEMORY_ATTR_NORMAL_WB, MAIR_NORMAL_WB); // Normal memory, write-back

  // Write the MAIR_EL1 register
  write_mair_el1(mair);
}

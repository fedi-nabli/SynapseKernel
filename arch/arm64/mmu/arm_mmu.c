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

#include <uart.h>

#include <arm_mmu.h>
#include <synapse/types.h>

// Temporary buffer for early boot string operations
static char temp_str_buffer[32];

// Convert a number to a string for UART output
static char* uint_to_str(uint64_t value)
{
  int i = 0;
  char* p = temp_str_buffer;

  do
  {
    temp_str_buffer[i++] = '0' + (value % 10);
    value /= 10;
  } while (value > 0 && i < 31);

  temp_str_buffer[i] = '\0';

  // Reverse the string
  int j = 0;
  i--;
  while (j < i)
  {
    char temp = p[j];
    p[j] = p[i];
    p[i] = temp;
    j++;
    i--;
  }

  return temp_str_buffer;
}

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

  // Clear all bits first
  sctlr = 0;

  // Set mandatory RES1 bits
  sctlr |= (1 << 11) | (1 << 20) | (1 << 22) | (1 << 28) | (1 << 29);

  // Write and verify
  write_sctlr_el1(sctlr);
  uint64_t check = read_sctlr_el1();
  uart_send_string("SCTLR_EL1 wrote: 0x");
  uart_send_string(uint_to_str(sctlr));
  uart_send_string("\nSCTLR_EL1 read: 0x");
  uart_send_string(uint_to_str(check));
  uart_send_string("\n");
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
  // Start with 0
  uint64_t tcr = 0;

  // Expected value for 48-bit VA:
  // 0x00000000B5193519
  
  // Bits for TTBR0_EL1 (low addresses)
  tcr |= (64 - 48);    // T0SZ=16, for 48-bit
  tcr |= (1 << 8);     // IRGN0=1, WB WA
  tcr |= (1 << 10);    // ORGN0=1, WB WA
  tcr |= (3 << 12);    // SH0=3, Inner
  tcr |= (0 << 14);    // TG0=0, 4KB

  // Bits for TTBR1_EL1 (high addresses)
  tcr |= (64 - 48) << 16; // T1SZ=16, for 48-bit
  tcr |= (1 << 24);    // IRGN1=1, WB WA
  tcr |= (1 << 26);    // ORGN1=1, WB WA
  tcr |= (3 << 28);    // SH1=3, Inner
  tcr |= (2ULL << 30); // TG1=2, 4KB

  // Physical Address Size
  tcr |= (2ULL << 32); // IPS=2, 40-bit PA

  // Write and verify
  write_tcr_el1(tcr);
  uint64_t check = read_tcr_el1();
  uart_send_string("TCR_EL1 wrote: 0x");
  uart_send_string(uint_to_str(tcr));
  uart_send_string("\nTCR_EL1 read: 0x");
  uart_send_string(uint_to_str(check));
  uart_send_string("\n");
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
  // Expected: 0xFF44040400
  uint64_t mair = 0;

  // Index 0: Normal memory, Write-Back
  mair |= 0xFFULL << (8 * 0);  // Normal WB RW WA

  // Index 1: Device-nGnRnE
  mair |= 0x00ULL << (8 * 1);  // Device

  // Index 2: Device-nGnRE
  mair |= 0x04ULL << (8 * 2);  // Device with early ack

  // Index 3: Normal NC
  mair |= 0x44ULL << (8 * 3);  // Normal NC

  // Write and verify
  write_mair_el1(mair);
  uint64_t check = read_mair_el1();
  uart_send_string("MAIR_EL1 wrote: 0x");
  uart_send_string(uint_to_str(mair));
  uart_send_string("\nMAIR_EL1 read: 0x");
  uart_send_string(uint_to_str(check));
  uart_send_string("\n");
}

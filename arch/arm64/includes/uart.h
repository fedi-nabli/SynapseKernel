/*
 * uart.h - UART peripheral definitions for bootloader and kernel
 *
 * This header defines UART register addresses and bits for use in
 * both assembly and C code during all boot stages.
 *
 * Author: Fedi Nabli
 * Date: 25 Feb 2025
 */

#ifndef __BOOT_UART_H_
#define __BOOT_UART_H_

#define UART_BASE       0x09000000 // Qemu virt PL011 UART base address

/* UART register offssets */
#define UART_DR         0x00 /* Data Register */
#define UART_FR         0x18 /* Flag Register */
#define UART_IBRD       0x24 /* Integer Baud Rate Divisor */
#define UART_FBRD       0x28 /* Fractional Baud Rate Divisor */
#define UART_LCR        0x2C /* Line Control Register */
#define UART_CR         0x30 /* Control Regoster */

/* Flag Register bits */
#define UART_FR_TXFF    (1 << 5) /* Transmit FIFO Full */
#define UART_FR_TXFE    (1 << 7) /* Transmit FIFO Empty */

/* Line Control register bits */
#define UART_LCR_WLEN_8 (3 << 5) /* 8 bit word length */
#define UART_LCT_FEN    (1 << 4) /* Enable FIFOs */

/* Control Register bits */
#define UART_CR_UARTEN  (1 << 0) /* UART ENable */
#define UART_CR_TXE     (1 << 8) /* Transmit enable */
#define UART_CR_RXE     (1 << 9) /* Receive enable */

#ifndef __ASSEMBLER__
#ifdef __cplusplus
extern "C" {
#endif

void uart_init(void);
void uart_send_char(char c);
void uart_send_string(const char* str);

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLER__ */

#endif /* __BOOT_UART_H_ */
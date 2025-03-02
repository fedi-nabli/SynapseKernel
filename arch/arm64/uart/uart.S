/*
 * uart.S - UART driver for QEMU virt PL011
 *
 * Basic UART functions for debugging and kernel messaging
 *
 * Author: Fedi Nabli
 * Date: 26 Feb 2025
 * Last Modified: 26 Feb 2025
 */

/* UART base address */
.equ UART_BASE,       0x09000000    /* Qemu virt PL011 UART base address */

/* UART register offssets */
.equ UART_DR,         0x00          /* Data Register */
.equ UART_FR,         0x18          /* Flag Register */
.equ UART_IBRD,       0x24          /* Integer Baud Rate Divisor */
.equ UART_FBRD,       0x28          /* Fractional Baud Rate Divisor */
.equ UART_LCR,        0x2C          /* Line Control Register */
.equ UART_CR,         0x30          /* Control Register */

/* Flag Register bits */
.equ UART_FR_TXFF,    (1 << 5)      /* Transmit FIFO Full */
.equ UART_FR_TXFE,    (1 << 7)      /* Transmit FIFO Empty */

/* Line Control register bits */
.equ UART_LCR_WLEN_8, (3 << 5)      /* 8 bit word length */
.equ UART_LCR_FEN,    (1 << 4)      /* Enable FIFOs */

/* Control Register bits */
.equ UART_CR_UARTEN,  (1 << 0)      /* UART Enable */
.equ UART_CR_TXE,     (1 << 8)      /* Transmit enable */
.equ UART_CR_RXE,     (1 << 9)      /* Receive enable */

.section ".text"

.global uart_init
.global uart_send_char
.global uart_send_string

/**
 * uart_init - Initialize the UART
 *
 * Sets up the UART for 115200 baud, 8N1 format
 */
uart_init:
  // Save link register
  stp x29, x30, [sp, #-16]!
  mov x29, sp

  // Get UART Base address
  ldr x0, =UART_BASE

  // Disable UART while configuring
  mov w1, #0
  str w1, [x0, #UART_CR]

  // Configure baud rate (assuming 24MHz UART clock, 115200 baud)
  // Divisor = UART_CLOCK / (16 * baud_rate)
  // 24000000 / (16 * 115200) = 13.0208...
  mov w1, #13
  str w1, [x0, #UART_IBRD]
  mov w1, #1
  str w1, [x0, #UART_FBRD]

  // Configure line control - 8 bits word length and FIFO enable
  mov w1, #UART_LCR_WLEN_8
  orr w1, w1, #UART_LCR_FEN
  str w1, [x0, #UART_LCR]

  // Enable UART, transmit and receive
  mov w1, #UART_CR_UARTEN
  orr w1, w1, #UART_CR_TXE
  orr w1, w1, #UART_CR_RXE
  str w1, [x0, #UART_CR]

  // Restore link register and return
  ldp x29, x30, [sp], #16
  ret

/** 
 * uart_send_char: Send a single character to the UART
 *
 * x0: Character to send
 */
uart_send_char:
  // Save link register and x1
  stp x29, x30, [sp, #-16]!
  stp x0, x1, [sp, #-16]!
  mov x29, sp

  // Handle newline (convert \n to \r\n)
  cmp w0, #10  // '\n'
  bne 1f

  // Send \r first
  mov w0, #13  // '\r'
  bl uart_send_char_raw

  // Restore x0 and then send \n
  ldr w0, [sp, #0]

1: // Send character
  bl uart_send_char_raw

  // Restore and return
  ldp x0, x1, [sp], #16
  ldp x29, x30, [sp], #16
  ret

/** 
 * uart_send_char_raw: Send a single character without newline handling
 *
 * x0: Character to send
 */
uart_send_char_raw:
  // Save registers
  stp x29, x30, [sp, #-16]!
  stp x0, x1, [sp, #-16]!
  mov x29, sp

  // Get UART base address
  ldr x1, =UART_BASE

  // Wait until transmit FIFO has space
1:
  ldr w2, [x1, #UART_FR]    // Load flag register
  and w2, w2, #UART_FR_TXFF // Check if transmit FIFO is full
  cbnz w2, 1b               // If full, keep waiting

  // Write character to data register
  strb w0, [x1, #UART_DR]

  // Restore and return
  ldp x0, x1, [sp], #16
  ldp x29, x30, [sp], #16
  ret

/** 
 * uart_send_string: Send a null-terminated string to the UART
 *
 * x0: Pointer to null-terminated string
 */
uart_send_string:
  stp x29, x30, [sp, #-16]!
  stp x0, x1, [sp, #-16]!
  mov x29, sp

  // Keep original string pointer in x1
  mov x1, x0

  // Loop through each character
1:
  ldrb w0, [x1], #1 // Load byte and increment pointer
  cbz w0, 2f        // If null terminator, exit loop

  // Send character
  bl uart_send_char

  // Continue loop
  b 1b

2:
  // Restore and return
  ldp x0, x1, [sp], #16
  ldp x29, x30, [sp], #16
  ret

/* Boot stage message strings */
.section ".rodata"
str_stage1:
  .asciz "First-stage bootloader started\n"
str_stage2:
  .asciz "Second-stage bootloader started\n"
str_kernel:
  .asciz "Kernel entry reached\n"
str_vectors:
  .asciz "Vector table initialized\n"
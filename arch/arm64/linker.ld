/* 
 * Linker script for ARM Robotics Kernel with AI Engine
 * Author: Fedi Nabli
 * Date: 26 Feb 2025
 * Last Modified: 26 Feb 2025
 */

OUTPUT_FORMAT("elf64-littleaarch64")
ENTRY(_start)

SECTIONS
{
  /* Kernel starts at 1MB for QEMU */
  . = 0x40000000;
  
  /* First the bootloader code */
  .text.boot : {
    *(.text.boot)
  }

  /* Second stage bootloader code */
  .text.boot.stage2 : ALIGN(16) {
    *(.text.boot.stage2)
  }
  
  /* Vector table - must be aligned to 2KB boundary for AArch64 */
  . = ALIGN(2048);
  .vector : {
    _vectors_start = .;
    vector_table = .;
    *(.vector)    /* Changed from .vectors to .vector */
    _vectors_end = .;
  }
  
  /* Main text section */
  .text : ALIGN(16) {
    *(.text)
  }
  
  /* Read-only data */
  .rodata : ALIGN(16) {
    *(.rodata)
    *(.rodata.*)
  }
  
  /* Read-write data (initialized) */
  .data : ALIGN(16) {
    _data_start = .;
    *(.data)
    _data_end = .;
  }
  
  /* Read-write data (uninitialized) */
  .bss : ALIGN(16) {
    __bss_start = .;
    *(.bss)
    *(COMMON)
    . = ALIGN(16);    /* Ensure end is aligned */
    __bss_end = .;
  }
  
  /* Stack section */
  .stack (NOLOAD) : ALIGN(16) {
    __stack_start = .;
    . = . + 16384;  /* 16KB stack */
    . = ALIGN(16);
    __stack_top = .;
  } = 0x0
  
  /* Mark the end of the kernel binary */
  _end = .;
  
  /* Discard all other sections */
  /DISCARD/ : {
    *(.comment)
    *(.note*)
    *(.eh_frame)
  }
}

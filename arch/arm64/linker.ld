/* 
 * Linker script for ARM Robotics Kernel with AI Engine
 * Author: Fedi Nabli
 * Date: 26 Feb 2025
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
  .text.boot.stage2 : {
    *(.text.boot.stage2)
  }
  
  /* Vector table - must be aligned to 2KB boundary for AArch64 */
  . = ALIGN(2048);
  .vectors : {
    _vectors_start = .;
    vector_table = .;
    *(.vectors)
    _vectors_end = .;
  }
  
  /* Main text section */
  .text : {
    . = ALIGN(8);
    *(.text)
  }
  
  /* Read-only data */
  .rodata : {
    . = ALIGN(8);
    *(.rodata)
    *(.rodata.*)
  }
  
  /* Read-write data (initialized) */
  .data : {
    . = ALIGN(8);
    _data_start = .;
    *(.data)
    _data_end = .;
  }
  
  /* Read-write data (uninitialized) */
  .bss : {
    . = ALIGN(8);
    __bss_start = .;
    *(.bss)
    *(COMMON)
    __bss_end = .;
  }
  
  /* Stack section */
  . = ALIGN(16);
  .stack : {
    __stack_start = .;
    . = . + 16384;  /* 16KB stack */
    __stack_top = .;
  }
  
  /* Mark the end of the kernel binary */
  _end = .;
  
  /* Discard all other sections */
  /DISCARD/ : {
    *(.comment)
    *(.note*)
  }
}

/*
 * boot_info.h - Boot information structure
 *
 * This header defines the boot information structure passed from
 * the bootloader to the kernel.
 * 
 * Author: Fedi Nabli
 * Date: 26 Feb 2025
 */

#ifndef __BOOT_INFO_
#define __BOOT_INFO_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef HAVE_STDINT
typedef unsigned long uint64_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
#else
#include <stdint.h>
#endif

// Magic value for verification "BOOT"
#define BOOT_INFO_MAGIC 0x424F4F54

typedef struct
{
  uint64_t magic; // Magic number: 0x424F4F54 "BOOT" in hex
  uint64_t architecture; // Architecture version
  uint64_t ram_size; // Total RAM size
  uint64_t kernel_size; // Size of kernel image
} boot_info_t;

#ifdef __cplusplus
}
#endif

#endif /* __BOOT_INFO_ */

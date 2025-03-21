/*
 * config.h - This file defines different kernel specific configuration
 * definitions (i.e max processes, console width...)
 *
 * Author: Fedi Nabli
 * Date: 2 Mar 2025
 * Last Modified: 2 Mar 2025
 */

#ifndef __KERNEL_CONFIG_H_
#define __KERNEL_CONFIG_H_

#define KERNEL_HEAP_BLOCK_SIZE 4096 // 4KB blocks

#define PAGE_SIZE 4096 // 4KB page size

// Memory pool configuration
#define AI_MEMORY_MIN_BLOCK_SIZE 64
#define AI_MEMORY_MAX_BLOCKS 4096

// Memory alignement constants
#define AI_MEMORY_ALIGN_SIMD 32 // For NEON/SVE instructions

#endif
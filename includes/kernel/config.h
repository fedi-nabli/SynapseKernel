/*
 * config.h - This file defines different kernel specific configuration
 * definitions (i.e max processes, console width...)
 *
 * Author: Fedi Nabli
 * Date: 2 Mar 2025
 * Last Modified: 8 Apr 2025
 */

#ifndef __KERNEL_CONFIG_H_
#define __KERNEL_CONFIG_H_

#define KERNEL_HEAP_BLOCK_SIZE 4096 // 4KB blocks

#define PAGE_SIZE 4096 // 4KB page size

#define MAX_PAGES (4 * 1024 * 1024) // 4M pages (covers 16GB RAM)

// Memory pool configuration
#define AI_MEMORY_MIN_BLOCK_SIZE 64
#define AI_MEMORY_MAX_BLOCKS 4096

// Memory alignement constants
#define AI_MEMORY_ALIGN_SIMD 32 // For NEON/SVE instructions

// Maximum number of memory regions we can track
#define MAX_MEMORY_REGIONS 32

// AI memory pool size (25% of total RAM by default)
#define AI_MEMORY_POOL_RATIO 4

// Maximum number of interrupt handlers
#define MAX_INTERRUPT_HANDLERS 128

#define SYNAPSE_MAX_PROCESSES 64
#define SYNAPSE_MAX_PROCESSES_ALLOCATIONS 128
#define SYNAPSE_PROCESS_STACK_SIZE (128 * 1024) // 128KB
#define SYNAPSE_MAX_PROCESS_NAME 64

#define CPU_FREQ_HZ 1000000000 // 1GHz

// Timer tick interval in ms
#define SCHEDULER_TICKS_MS 10

#endif
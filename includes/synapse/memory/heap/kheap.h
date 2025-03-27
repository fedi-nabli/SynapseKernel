/*
 * kheap.h - This file defines different kernel heap
 * functions for memory
 *
 * Author: Fedi Nabli
 * Date: 6 Mar 2025
 * Last Modified: 27 Mar 2025
 */

#ifndef __SYNAPSE_MEMORY_KHEAP_H_
#define __SYNAPSE_MEMORY_KHEAP_H_

#include <synapse/types.h>

/**
 * @brief Initializes the kernel heap with dynamic size based on
 * available RAM
 * 
 * @author Fedi Nabli
 * @date 6 Mar 2025
 * 
 * @param ram_size total amount of available ram from boot_info structure 
 */
void kheap_init(size_t ram_size);

/**
 * @brief Allocated requested amount in the memory
 * and returns the address
 * 
 * @author Fedi Nabli
 * @date 6 Mar 2025
 * 
 * @param size amount of memory needed in bytes 
 * @return void* start address of the memory allocated
 */
void* kmalloc(size_t size);

/**
 * @brief Allocates memory and initializes it to zero
 * 
 * @param size Amount of memory needed in bytes
 * @return void* Start address of the zero-initialized memory allocated
 * 
 * @author Fedi Nabli
 * @date 27 Mar 2025
 */
void* kzalloc(size_t size);

/**
 * @brief Frees memory region previously allocated to ptr
 * 
 * @author Fedi Nabli
 * @date 6 Mar 2025
 * 
 * @param ptr the address of the variable
 */
void kfree(void* ptr);

#endif

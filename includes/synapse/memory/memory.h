/*
 * memory.h - This file defines different global memory
 * functions like the standard library
 *
 * Author: Fedi Nabli
 * Date: 2 Mar 2025
 * Last Modified: 2 Mar 2025
 */

#ifndef __SYNAPSE_MEMORY_H_
#define __SYNAPSE_MEMORY_H_

#include <synapse/types.h>

/**
 * @brief fills a memory address with the givven character
 * 
 * @param ptr void* address of the memory region
 * @param c character to be put into memory
 * @param n length of ptr
 * @return void* address of the memory after being filled
 */
void* memset(void* ptr, int c, size_t n);

#endif
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

/**
 * @brief compares two memory regions
 * 
 * @param s1 void* address of the first memory region
 * @param s2 void* address of the second memory region
 * @param count number of bytes to compare
 * @return int 0 if the memory regions are equal, a negative value if the first differing byte in s1 is less than the corresponding byte in s2, and a positive value if it is greater
 * 
 * @author Fedi Nabli
 * @date 21 Mar 2025
 */
int memcmp(void* s1, void* s2, int count);

/**
 * @brief copies memory from source to destination
 * 
 * @param dest void* address of the destination memory region
 * @param src void* address of the source memory region
 * @param len number of bytes to copy
 * @return void* address of the destination memory region
 * 
 * @author Fedi Nabli
 * @date 21 Mar 2025
 */
void* memcpy(void* dest, void* src, int len);

#endif
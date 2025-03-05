/*
 * memory.c - This file implementes different global memory
 * functions found in memory.h
 *
 * Author: Fedi Nabli
 * Date: 2 Mar 2025
 * Last Modified: 2 Mar 2025
 */

#include <synapse/memory/memory.h>

void* memset(void* ptr, int c, size_t n)
{
  char* c_ptr = (char*) ptr;
  for (size_t i = 0; i < n; i++)
  {
    c_ptr[i] = (char) c;
  }

  return ptr;
}
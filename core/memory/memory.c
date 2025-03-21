/*
 * memory.c - This file implementes different global memory
 * functions found in memory.h
 *
 * Author: Fedi Nabli
 * Date: 2 Mar 2025
 * Last Modified: 21 Mar 2025
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

int memcmp(void* s1, void* s2, int count)
{
  char* c1 = s1;
  char* c2 = s2;
  while (count-- > 0)
  {
    if (*c1++ != *c2++)
    {
      return c1[-1] < c2[-1] ? -1 : 1;
    }
  }

  return 0;
}

void* memcpy(void* dest, void* src, int len)
{
  char* d = dest;
  char* s = src;
  while (len--)
  {
    *d++ = *s++;
  }

  return dest;
}

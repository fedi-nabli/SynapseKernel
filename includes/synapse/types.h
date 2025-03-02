/*
 * types.h - This file defines different types to be used by the kernel
 *
 * Author: Fedi Nabli
 * Date: 29 Feb 2025
 * Last Modified: 1 Mar 2025
 */

#ifndef __SYNAPSE_TYPES_H_
#define __SYNAPSE_TYPES_H_

/* Integer types */
typedef unsigned char   uint8_t;
typedef unsigned short  uint16_t;
typedef unsigned int    uint32_t;
typedef unsigned long   uint64_t;

typedef signed char     int8_t;
typedef signed short    int16_t;
typedef signed int      int32_t;
typedef signed long     int64_t;

/* Size types */
typedef unsigned long   size_t;
typedef signed long     ssize_t;
typedef unsigned long   uintptr_t;
typedef signed long     intptr_t;

/* Fixed width types with limits */
#define UINT8_MAX   0xFF
#define UINT16_MAX  0xFFFF
#define UINT32_MAX  0xFFFFFFFF
#define UINT64_MAX  0xFFFFFFFFFFFFFFFF

#define INT8_MAX    0x7F
#define INT16_MAX   0x7FFF
#define INT32_MAX   0x7FFFFFFF
#define INT64_MAX   0x7FFFFFFFFFFFFFFF

#define INT8_MIN    (-INT8_MAX - 1)
#define INT16_MIN   (-INT16_MAX - 1)
#define INT32_MIN   (-INT32_MAX - 1)
#define INT64_MIN   (-INT64_MAX - 1)

#endif
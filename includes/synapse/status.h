/*
 * status.h - This file defines the different error codes
 *
 * Author: Fedi Nabli
 * Date: 1 Mar 2025
 * Last Modified: 8 Apr 2025
 */

#ifndef __SYNAPSE_STATUS_H_
#define __SYNAPSE_STATUS_H_

#define EOK 0 // No error
#define EIO 1
#define EINVARG 2
#define ENOMEM 3
#define EMMU 4
#define ENOMAP 5
#define EINVAL 6
#define ENOTREADY 7
#define EFAULT 8
#define NOFREERGE 9 // No free range
#define EINUSE 10
#define ENOTASK 11
#define EPMAX 12
#define EINVSYSCALL 13
#define ESYSCALL 14
#define ENOENT 15

#endif
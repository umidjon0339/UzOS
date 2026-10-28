#ifndef SYS_SCHED_SETAFFINITY_H
#define SYS_SCHED_SETAFFINITY_H

/*
 * sys_sched_setaffinity.h - sched_setaffinity(): which CPUs a process may use.
 */

#include <stdint.h>

#define SYS_SCHED_SETAFFINITY 33    // EAX = 33, EBX = pid (0 = me), ECX = mask size in bytes, EDX = &mask

// Set the CPU mask of 'pid' from the 'size' bytes at 'mask' (bit n =
// CPU n). 0 = done, -1 = refused (see core/affinity).
int sys_sched_setaffinity(uint32_t pid, uint32_t size, const uint8_t *mask);

#endif

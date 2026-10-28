#ifndef SYS_SCHED_GETAFFINITY_H
#define SYS_SCHED_GETAFFINITY_H

/*
 * sys_sched_getaffinity.h - sched_getaffinity(): which CPUs a process may use.
 */

#include <stdint.h>

#define SYS_SCHED_GETAFFINITY 34    // EAX = 34, EBX = pid (0 = me), ECX = buffer size, EDX = &mask

// Write the CPU mask of 'pid' into 'mask' ('size' bytes, at least 4).
// Returns the bytes written (4), or -1.
int sys_sched_getaffinity(uint32_t pid, uint32_t size, uint8_t *mask);

#endif

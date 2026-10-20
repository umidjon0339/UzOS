#ifndef SYS_GETPRIORITY_H
#define SYS_GETPRIORITY_H

/*
 * sys_getpriority.h - getpriority(): read a process's priority level.
 */

#include <stdint.h>

#define SYS_GETPRIORITY 22  // int 0x80 with EAX = 22, EBX = pid (0 = me); level back in EAX

// The base level of 'pid' (0 = the caller): 0-7, or -1 if no such process.
int32_t sys_getpriority(uint32_t pid);

#endif

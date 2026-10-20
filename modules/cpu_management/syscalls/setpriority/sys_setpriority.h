#ifndef SYS_SETPRIORITY_H
#define SYS_SETPRIORITY_H

/*
 * sys_setpriority.h - setpriority(): change how soon a process gets the CPU.
 */

#include <stdint.h>

#define SYS_SETPRIORITY 21  // int 0x80 with EAX = 21, EBX = pid (0 = me), ECX = level

// Set 'pid' (0 = the caller) to 'level' (1-7). 0 = done, -1 = refused.
int32_t sys_setpriority(uint32_t pid, int32_t level);

#endif

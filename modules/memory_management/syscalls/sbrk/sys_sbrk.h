#ifndef SYS_SBRK_H
#define SYS_SBRK_H

/*
 * sys_sbrk.h - sbrk(): grow or shrink the program's heap.
 */

#include <stdint.h>

#define SYS_SBRK    5                   // int 0x80 with EAX = 5, EBX = bytes (may be < 0)
#define SBRK_FAILED ((void *)-1)        // same error value as Linux

void *sys_sbrk(int32_t increment);

#endif

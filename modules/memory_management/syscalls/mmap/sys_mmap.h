#ifndef SYS_MMAP_H
#define SYS_MMAP_H

/*
 * sys_mmap.h - mmap(): ask for a fresh block of memory.
 */

#include <stdint.h>

#define SYS_MMAP 6      // int 0x80 with EAX = 6, EBX = length; address back in EAX

// 'length' bytes (rounded up to whole pages) of zeroed, user-writable
// memory at an address the kernel picks. Returns NULL on failure.
void *sys_mmap(uint32_t length);

#endif

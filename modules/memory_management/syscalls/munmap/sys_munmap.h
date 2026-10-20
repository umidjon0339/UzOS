#ifndef SYS_MUNMAP_H
#define SYS_MUNMAP_H

/*
 * sys_munmap.h - munmap(): give an mmap() region back.
 */

#include <stdint.h>

#define SYS_MUNMAP 7    // int 0x80 with EAX = 7, EBX = address, ECX = length

// Returns 0 on success, -1 if refused.
int sys_munmap(void *addr, uint32_t length);

#endif

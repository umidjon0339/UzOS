#ifndef SYS_SEEK_H
#define SYS_SEEK_H

/*
 * sys_seek.h - seek(): move a descriptor's read/write position.
 */

#include <stdint.h>

#define SYS_SEEK 18     // int 0x80 with EAX = 18, EBX = fd, ECX = offset, EDX = whence

#define SEEK_SET 0      // offset from the start of the file
#define SEEK_CUR 1      // offset from the current position
#define SEEK_END 2      // offset from the end of the file

// The new position, or -1.
int sys_seek(int fd, int32_t offset, int whence);

#endif

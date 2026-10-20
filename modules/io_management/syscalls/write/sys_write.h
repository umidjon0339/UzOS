#ifndef SYS_WRITE_H
#define SYS_WRITE_H

/*
 * sys_write.h - write(): copy bytes from a buffer to a descriptor.
 */

#include <stdint.h>

#define SYS_WRITE 16    // int 0x80 with EAX = 16, EBX = fd, ECX = buffer, EDX = count

// Bytes written, or -1.
int sys_write(int fd, const void *buf, uint32_t count);

#endif

#ifndef SYS_READ_H
#define SYS_READ_H

/*
 * sys_read.h - read(): copy bytes from a descriptor into a buffer.
 */

#include <stdint.h>

#define SYS_READ 15     // int 0x80 with EAX = 15, EBX = fd, ECX = buffer, EDX = count

// Bytes read (0 = end of file / no keys waiting), or -1.
int sys_read(int fd, void *buf, uint32_t count);

#endif

#ifndef SYS_OPEN_H
#define SYS_OPEN_H

/*
 * sys_open.h - open(): get a file descriptor for a file.
 */

#include <stdint.h>

#define SYS_OPEN 14     // int 0x80 with EAX = 14, EBX = path, ECX = flags

// Flags: what you want to do with the file. Combine with |.
#define O_READ   0x01   // read() allowed
#define O_WRITE  0x02   // write() allowed
#define O_CREATE 0x04   // create the file if it does not exist
#define O_TRUNC  0x08   // empty the file first (needs O_WRITE)
#define O_APPEND 0x10   // every write() goes to the end

// The new descriptor (3 or more), or -1.
int sys_open(const char *path, uint32_t flags);

#endif

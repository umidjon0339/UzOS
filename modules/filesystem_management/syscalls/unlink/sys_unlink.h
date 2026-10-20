#ifndef SYS_UNLINK_H
#define SYS_UNLINK_H

/*
 * sys_unlink.h - unlink(): delete a file or an empty directory.
 */

#include <stdint.h>

#define SYS_UNLINK 10    // int 0x80 with EAX = 10, EBX = path

int sys_unlink(const char *path);             // 0 or -1

#endif

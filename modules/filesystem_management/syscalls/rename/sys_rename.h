#ifndef SYS_RENAME_H
#define SYS_RENAME_H

/*
 * sys_rename.h - rename(): move or rename a file or directory.
 */

#include <stdint.h>

#define SYS_RENAME 13    // int 0x80 with EAX = 13, EBX = from, ECX = to

int sys_rename(const char *from, const char *to);   // 0 or -1

#endif

#ifndef SYS_CHDIR_H
#define SYS_CHDIR_H

/*
 * sys_chdir.h - chdir(): change the current directory.
 */

#include <stdint.h>

#define SYS_CHDIR 11    // int 0x80 with EAX = 11, EBX = path

int sys_chdir(const char *path);              // 0 or -1

#endif

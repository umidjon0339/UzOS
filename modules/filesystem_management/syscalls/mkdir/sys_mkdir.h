#ifndef SYS_MKDIR_H
#define SYS_MKDIR_H

/*
 * sys_mkdir.h - mkdir(): create a directory.
 */

#include <stdint.h>

#define SYS_MKDIR 9    // int 0x80 with EAX = 9, EBX = path

int sys_mkdir(const char *path);              // 0 or -1

#endif

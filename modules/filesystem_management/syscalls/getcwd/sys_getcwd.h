#ifndef SYS_GETCWD_H
#define SYS_GETCWD_H

/*
 * sys_getcwd.h - getcwd(): the current directory as a path.
 */

#include <stdint.h>

#define SYS_GETCWD 12    // int 0x80 with EAX = 12, EBX = buffer, ECX = size

int sys_getcwd(char *buf, uint32_t size);     // 0 or -1

#endif

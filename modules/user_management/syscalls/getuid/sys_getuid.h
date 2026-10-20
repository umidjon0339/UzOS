#ifndef SYS_GETUID_H
#define SYS_GETUID_H

/*
 * sys_getuid.h - getuid(): which user am I?
 */

#include <stdint.h>

#define SYS_GETUID 23   // int 0x80 with EAX = 23; the uid comes back in EAX

uint32_t sys_getuid(void);

#endif

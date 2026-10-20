#ifndef SYS_SETUID_H
#define SYS_SETUID_H

/*
 * sys_setuid.h - setuid(): run as another user from now on.
 */

#include <stdint.h>

#define SYS_SETUID 24   // int 0x80 with EAX = 24, EBX = uid; 0 or -1 back in EAX

// Make the calling process belong to 'uid'. 0 = done, -1 = refused.
int32_t sys_setuid(uint32_t uid);

#endif

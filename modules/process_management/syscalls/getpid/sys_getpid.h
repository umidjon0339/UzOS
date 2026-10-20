#ifndef SYS_GETPID_H
#define SYS_GETPID_H

/*
 * sys_getpid.h - getpid(): "what is my process ID?"
 */

#include <stdint.h>

#define SYS_GETPID 2    // int 0x80 with EAX = 2, PID comes back in EAX

uint32_t sys_getpid(void);

#endif

#ifndef SYS_GETPID_H
#define SYS_GETPID_H

/*
 * sys_getpid.h - getpid(): "what is my process ID?"
 */

#include <stdint.h>

#define SYS_GETPID 2    // int 0x80 with EAX = 2; the PID comes back in EAX

// The PROCESS id of the caller: its thread group id. All threads made
// by clone(CLONE_THREAD) share it (see gettid for each thread's own id).
uint32_t sys_getpid(void);

#endif

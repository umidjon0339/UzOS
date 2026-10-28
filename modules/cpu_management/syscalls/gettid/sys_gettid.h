#ifndef SYS_GETTID_H
#define SYS_GETTID_H

/*
 * sys_gettid.h - gettid(): "what is MY thread's ID?"
 */

#include <stdint.h>

#define SYS_GETTID 42   // EAX = 42; the thread id comes back in EAX

// The id of the calling thread: its own pid in the process table.
uint32_t sys_gettid(void);

#endif

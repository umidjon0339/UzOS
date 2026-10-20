#ifndef SYS_SLEEP_H
#define SYS_SLEEP_H

/*
 * sys_sleep.h - sleep(): pause the calling process for a while.
 */

#include <stdint.h>

#define SYS_SLEEP 3     // int 0x80 with EAX = 3, EBX = milliseconds

// Sleep the full 'ms', whatever happens (uninterruptible).
void sys_sleep(uint32_t ms);

// Sleep 'ms', but wake at once if a signal arrives (e.g. Ctrl+C).
// Returns 1 if it slept the whole time, 0 if a signal ended it early
// (or was already pending: then it does not sleep at all).
// Kernel only: no syscall number, ring 3 programs get no signals yet.
int sys_sleep_interruptible(uint32_t ms);

#endif

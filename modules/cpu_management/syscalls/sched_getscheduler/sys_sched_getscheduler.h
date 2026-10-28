#ifndef SYS_SCHED_GETSCHEDULER_H
#define SYS_SCHED_GETSCHEDULER_H

/*
 * sys_sched_getscheduler.h - sched_getscheduler(): which policy a process has.
 */

#include <stdint.h>

#define SYS_SCHED_GETSCHEDULER 27   // EAX = 27, EBX = pid (0 = me); policy back in EAX

// The policy of 'pid' (SCHED_* from core/policy, with SCHED_RESET_ON_FORK
// OR'd in if set), or -1 if there is no such process.
int sys_sched_getscheduler(uint32_t pid);

#endif

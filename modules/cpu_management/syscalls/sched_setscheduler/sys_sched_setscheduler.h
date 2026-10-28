#ifndef SYS_SCHED_SETSCHEDULER_H
#define SYS_SCHED_SETSCHEDULER_H

/*
 * sys_sched_setscheduler.h - sched_setscheduler(): choose the scheduling policy.
 */

#include <stdint.h>
#include "modules/cpu_management/core/policy/policy.h"     // sched_param_t, SCHED_*

#define SYS_SCHED_SETSCHEDULER 26   // EAX = 26, EBX = pid (0 = me), ECX = policy, EDX = &param

// Put 'pid' under 'policy' (SCHED_OTHER, SCHED_FIFO, ...; may have
// SCHED_RESET_ON_FORK OR'd in) with param->sched_priority.
// 0 = done, -1 = refused.
int sys_sched_setscheduler(uint32_t pid, int policy, const sched_param_t *param);

#endif

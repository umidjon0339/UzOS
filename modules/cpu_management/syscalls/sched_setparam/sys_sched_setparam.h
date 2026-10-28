#ifndef SYS_SCHED_SETPARAM_H
#define SYS_SCHED_SETPARAM_H

/*
 * sys_sched_setparam.h - sched_setparam(): change the priority, keep the policy.
 */

#include <stdint.h>
#include "modules/cpu_management/core/policy/policy.h"     // sched_param_t

#define SYS_SCHED_SETPARAM 28   // EAX = 28, EBX = pid (0 = me), ECX = &param

// Give 'pid' the real-time priority param->sched_priority under the
// policy it already has. 0 = done, -1 = refused.
int sys_sched_setparam(uint32_t pid, const sched_param_t *param);

#endif

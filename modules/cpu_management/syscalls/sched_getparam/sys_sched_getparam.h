#ifndef SYS_SCHED_GETPARAM_H
#define SYS_SCHED_GETPARAM_H

/*
 * sys_sched_getparam.h - sched_getparam(): read the real-time priority.
 */

#include <stdint.h>
#include "modules/cpu_management/core/policy/policy.h"     // sched_param_t

#define SYS_SCHED_GETPARAM 29   // EAX = 29, EBX = pid (0 = me), ECX = &param

// Fill *param for 'pid'. 0 = done, -1 = no such process.
int sys_sched_getparam(uint32_t pid, sched_param_t *param);

#endif

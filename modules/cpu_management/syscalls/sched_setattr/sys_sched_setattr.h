#ifndef SYS_SCHED_SETATTR_H
#define SYS_SCHED_SETATTR_H

/*
 * sys_sched_setattr.h - sched_setattr(): every scheduling setting in one struct.
 */

#include <stdint.h>
#include "modules/cpu_management/core/policy/policy.h"     // sched_attr_t

#define SYS_SCHED_SETATTR 35    // EAX = 35, EBX = pid (0 = me), ECX = &attr, EDX = flags (0)

// Apply *attr (policy, nice, rt priority, reset-on-fork) to 'pid'.
// 0 = done, -1 = refused.
int sys_sched_setattr(uint32_t pid, const sched_attr_t *attr, uint32_t flags);

#endif

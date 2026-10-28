#ifndef SYS_SCHED_GETATTR_H
#define SYS_SCHED_GETATTR_H

/*
 * sys_sched_getattr.h - sched_getattr(): read every scheduling setting.
 */

#include <stdint.h>
#include "modules/cpu_management/core/policy/policy.h"     // sched_attr_t

#define SYS_SCHED_GETATTR 36    // EAX = 36, EBX = pid (0 = me), ECX = &attr, EDX = size, ESI = flags (0)

// Fill *attr ('size' bytes, at least 48) for 'pid'. 0 = done, -1 = error.
int sys_sched_getattr(uint32_t pid, sched_attr_t *attr, uint32_t size, uint32_t flags);

#endif

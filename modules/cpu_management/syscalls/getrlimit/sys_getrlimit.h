#ifndef SYS_GETRLIMIT_H
#define SYS_GETRLIMIT_H

/*
 * sys_getrlimit.h - getrlimit(): read one of my resource limits.
 */

#include <stdint.h>
#include "modules/cpu_management/core/rlimit/rlimit.h"     // rlimit_t, RLIMIT_*

#define SYS_GETRLIMIT 37    // EAX = 37, EBX = resource, ECX = &rlimit

// Fill *limit with the caller's soft and hard limit of 'resource'.
// 0 = done, -1 = no such resource.
int sys_getrlimit(int resource, rlimit_t *limit);

#endif

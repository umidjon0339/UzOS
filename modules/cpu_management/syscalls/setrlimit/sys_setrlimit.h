#ifndef SYS_SETRLIMIT_H
#define SYS_SETRLIMIT_H

/*
 * sys_setrlimit.h - setrlimit(): change one of my resource limits.
 */

#include <stdint.h>
#include "modules/cpu_management/core/rlimit/rlimit.h"     // rlimit_t, RLIMIT_*

#define SYS_SETRLIMIT 38    // EAX = 38, EBX = resource, ECX = &rlimit

// Set the caller's limit of 'resource' to *limit. 0 = done, -1 =
// refused (soft above hard, raising the hard limit without
// CAP_RESOURCE, or a limit UzOS does not use).
int sys_setrlimit(int resource, const rlimit_t *limit);

#endif

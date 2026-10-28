#ifndef SYS_PRLIMIT64_H
#define SYS_PRLIMIT64_H

/*
 * sys_prlimit64.h - prlimit64(): read and/or set a limit of ANY process.
 */

#include <stdint.h>
#include "modules/cpu_management/core/rlimit/rlimit.h"     // rlimit64_t, RLIMIT_*

#define SYS_PRLIMIT64 39    // EAX = 39, EBX = pid (0 = me), ECX = resource, EDX = &new (or 0), ESI = &old (or 0)

// If 'old' is not NULL, fill it with the current limit; then, if 'new'
// is not NULL, set it. 0 = done, -1 = refused / no such process.
int sys_prlimit64(uint32_t pid, int resource, const rlimit64_t *new_limit,
                  rlimit64_t *old_limit);

#endif

/*
 * sys_getrlimit.c - getrlimit(): one limit of the caller.
 *
 *     rlimit_t l;
 *     getrlimit(RLIMIT_NOFILE, &l);     // l.cur = 16, l.max = 16
 *
 * A limit UzOS does not use reads as RLIM_INFINITY ("unlimited").
 *
 * Uses: rlimit_get (core/rlimit), the current process (process_management).
 */

#include "modules/cpu_management/syscalls/getrlimit/sys_getrlimit.h"
#include "modules/process_management/core/scheduler/scheduler.h"    // the current process

int sys_getrlimit(int resource, rlimit_t *limit) {
    if (!limit)
        return -1;
    return rlimit_get(scheduler_get_current_process(), resource, limit) ? 0 : -1;
}

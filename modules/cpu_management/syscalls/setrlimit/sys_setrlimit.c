/*
 * sys_setrlimit.c - setrlimit(): lower (or, as root, raise) one limit.
 *
 *     rlimit_t l = { .cur = 2, .max = 2 };
 *     setrlimit(RLIMIT_CPU, &l);    // after 2 s of CPU: killed
 *
 * A lowered hard limit cannot be raised again without CAP_RESOURCE:
 * that is what makes it a real limit and not a suggestion.
 *
 * Uses: rlimit_set (core/rlimit), the current process (process_management).
 */

#include "modules/cpu_management/syscalls/setrlimit/sys_setrlimit.h"
#include "modules/process_management/core/scheduler/scheduler.h"    // the current process

int sys_setrlimit(int resource, const rlimit_t *limit) {
    process_t *self = scheduler_get_current_process();
    if (!limit)
        return -1;
    return rlimit_set(self, self, resource, limit) ? 0 : -1;
}

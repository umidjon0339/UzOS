/*
 * sys_prlimit64.c - prlimit64(): getrlimit + setrlimit for another process.
 *
 *     prlimit64(7, RLIMIT_CPU, &new, &old)    the `prlimit` tool's call
 *
 * Two differences from setrlimit():
 *   - it names a process: only your own, unless you hold CAP_RESOURCE
 *   - the values are 64-bit ("64"), so limits above 4 GiB fit. UzOS
 *     keeps 32-bit limits: anything above 0xFFFFFFFE means unlimited.
 *
 * Uses: rlimit_get / rlimit_set (core/rlimit), process_get_by_pid and
 *       the current process (process_management).
 */

#include "modules/cpu_management/syscalls/prlimit64/sys_prlimit64.h"
#include "modules/process_management/core/scheduler/scheduler.h"    // the current process

static uint32_t to_32(uint64_t value) {
    return value >= RLIM_INFINITY ? RLIM_INFINITY : (uint32_t)value;
}

static uint64_t to_64(uint32_t value) {
    return value == RLIM_INFINITY ? RLIM64_INFINITY : value;
}

int sys_prlimit64(uint32_t pid, int resource, const rlimit64_t *new_limit,
                  rlimit64_t *old_limit) {
    process_t *self = scheduler_get_current_process();
    process_t *target = pid ? process_get_by_pid(pid) : self;
    rlimit_t now;

    if (!target || !rlimit_get(target, resource, &now))
        return -1;
    if (old_limit) {
        old_limit->cur = to_64(now.cur);
        old_limit->max = to_64(now.max);
    }
    if (new_limit) {
        rlimit_t set = { to_32(new_limit->cur), to_32(new_limit->max) };
        if (!rlimit_set(self, target, resource, &set))
            return -1;
    }
    return 0;
}

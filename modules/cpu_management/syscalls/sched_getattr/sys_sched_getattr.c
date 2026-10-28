/*
 * sys_sched_getattr.c - sched_getattr(): policy, nice, rt priority, flags.
 *
 * Uses: SCHED_* (core/policy), process_get_by_pid and the current
 *       process (process_management), memset (lib/string).
 */

#include "modules/cpu_management/syscalls/sched_getattr/sys_sched_getattr.h"
#include "modules/process_management/core/scheduler/scheduler.h"    // the current process
#include "lib/string/string.h"                                      // memset

int sys_sched_getattr(uint32_t pid, sched_attr_t *attr, uint32_t size, uint32_t flags) {
    process_t *p = pid ? process_get_by_pid(pid) : scheduler_get_current_process();
    if (!p || !attr || size < SCHED_ATTR_SIZE || flags != 0)
        return -1;

    memset(attr, 0, SCHED_ATTR_SIZE);
    attr->size           = SCHED_ATTR_SIZE;
    attr->sched_policy   = p->policy;
    attr->sched_flags    = p->reset_on_fork ? SCHED_FLAG_RESET_ON_FORK : 0;
    attr->sched_nice     = p->nice;
    attr->sched_priority = p->rt_priority;
    return 0;
}

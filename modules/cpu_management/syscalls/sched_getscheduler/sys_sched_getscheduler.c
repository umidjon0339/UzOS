/*
 * sys_sched_getscheduler.c - sched_getscheduler(): read a process's policy.
 *
 * Reading is allowed for every process (as in Linux): it changes nothing.
 *
 * Uses: SCHED_RESET_ON_FORK (core/policy), process_get_by_pid and the
 *       current process (process_management).
 */

#include "modules/cpu_management/syscalls/sched_getscheduler/sys_sched_getscheduler.h"
#include "modules/cpu_management/core/policy/policy.h"              // SCHED_RESET_ON_FORK
#include "modules/process_management/core/scheduler/scheduler.h"    // the current process

int sys_sched_getscheduler(uint32_t pid) {
    process_t *p = pid ? process_get_by_pid(pid) : scheduler_get_current_process();
    if (!p)
        return -1;
    return p->policy | (p->reset_on_fork ? SCHED_RESET_ON_FORK : 0);
}

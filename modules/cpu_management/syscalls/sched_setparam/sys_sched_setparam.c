/*
 * sys_sched_setparam.c - sched_setparam(): a new priority, the same policy.
 *
 * Only real-time policies have a priority here (1..99); for SCHED_OTHER
 * the only valid value is 0, and the "priority" of a normal process is
 * its nice value (setpriority).
 *
 * Uses: policy_set (core/policy), process_get_by_pid and the current
 *       process (process_management).
 */

#include "modules/cpu_management/syscalls/sched_setparam/sys_sched_setparam.h"
#include "modules/process_management/core/scheduler/scheduler.h"    // the current process

int sys_sched_setparam(uint32_t pid, const sched_param_t *param) {
    process_t *self = scheduler_get_current_process();
    process_t *target = pid ? process_get_by_pid(pid) : self;
    if (!param || !target)
        return -1;

    // The same policy, a new priority: policy_set checks it all again.
    return policy_set(self, target->pid, target->policy, param->sched_priority,
                      target->reset_on_fork) ? 0 : -1;
}

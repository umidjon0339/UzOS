/*
 * sys_sched_setscheduler.c - sched_setscheduler(): policy + priority in one call.
 *
 *     struct sched_param p = { .sched_priority = 20 };
 *     sched_setscheduler(0, SCHED_FIFO, &p);     // me: real-time, level 20
 *     sched_setscheduler(0, SCHED_OTHER, &p0);   // back to normal (priority 0)
 *
 * The checks (a known policy, a priority in its range, the right to
 * real-time) are the defence priority_bounds, inside policy_set().
 *
 * Uses: policy_set (core/policy), the current process (process_management).
 */

#include "modules/cpu_management/syscalls/sched_setscheduler/sys_sched_setscheduler.h"
#include "modules/process_management/core/scheduler/scheduler.h"    // scheduler_get_current_process

int sys_sched_setscheduler(uint32_t pid, int policy, const sched_param_t *param) {
    process_t *self = scheduler_get_current_process();
    if (!param || !self)
        return -1;

    int reset = (policy & SCHED_RESET_ON_FORK) != 0;
    policy &= ~SCHED_RESET_ON_FORK;
    if (pid == 0)
        pid = self->pid;

    return policy_set(self, pid, policy, param->sched_priority, reset) ? 0 : -1;
}

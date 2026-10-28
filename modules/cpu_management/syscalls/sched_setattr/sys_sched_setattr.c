/*
 * sys_sched_setattr.c - sched_setattr(): the newest, most complete call.
 *
 * sched_setscheduler() cannot set a nice value, and setpriority()
 * cannot set a policy. sched_setattr() takes them all at once:
 *
 *     sched_attr_t a = { .size = 48, .sched_policy = SCHED_BATCH,
 *                        .sched_nice = 5 };
 *     sched_setattr(0, &a, 0);
 *
 * attr->size tells which version of the struct the caller knows, so
 * old programs keep working when Linux adds fields. UzOS knows the
 * first version (48 bytes) and refuses SCHED_DEADLINE.
 *
 * Both parts are checked by the defence priority_bounds (policy_set,
 * priority_set_nice); nothing is changed unless both are allowed.
 *
 * Uses: policy_set (core/policy), priority_set_nice (core/priority),
 *       def_nice_change_allowed / def_policy_change_allowed (defence
 *       priority_bounds), the process table (process_management).
 */

#include "modules/cpu_management/syscalls/sched_setattr/sys_sched_setattr.h"
#include "modules/cpu_management/core/priority/priority.h"          // priority_set_nice
#include "modules/cpu_management/security/priority_abuse/defence/priority_bounds/def_priority_bounds.h"
#include "modules/process_management/core/scheduler/scheduler.h"    // the current process

int sys_sched_setattr(uint32_t pid, const sched_attr_t *attr, uint32_t flags) {
    process_t *self = scheduler_get_current_process();
    process_t *target = pid ? process_get_by_pid(pid) : self;
    if (!attr || !target || flags != 0 || attr->size < SCHED_ATTR_SIZE)
        return -1;

    int policy = attr->sched_policy;
    int rt = attr->sched_priority;
    int nice = policy_is_realtime(policy) ? target->nice : attr->sched_nice;
    int reset = (attr->sched_flags & SCHED_FLAG_RESET_ON_FORK) != 0;

    // Check both halves first, so a refused nice leaves the policy alone.
    if (!def_policy_change_allowed(self, target, policy, rt) ||
        !def_nice_change_allowed(self, target, nice))
        return -1;

    policy_set(self, target->pid, policy, rt, reset);
    priority_set_nice(self, target->pid, nice);
    return 0;
}

/*
 * sys_sched_get_priority_max.c - sched_get_priority_max(): the top of a policy's range.
 *
 * Uses: policy_priority_max (core/policy).
 */

#include "modules/cpu_management/syscalls/sched_get_priority_max/sys_sched_get_priority_max.h"
#include "modules/cpu_management/core/policy/policy.h"      // policy_priority_max

int sys_sched_get_priority_max(int policy) {
    return policy_priority_max(policy);
}

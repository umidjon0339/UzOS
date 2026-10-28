/*
 * sys_sched_get_priority_min.c - sched_get_priority_min(): the bottom of a policy's range.
 *
 * Programs must not guess the range ("is it 1..99 or 0..99?"): they ask.
 * A portable program picks e.g. min + 10 for a modest real-time priority.
 *
 * Uses: policy_priority_min (core/policy).
 */

#include "modules/cpu_management/syscalls/sched_get_priority_min/sys_sched_get_priority_min.h"
#include "modules/cpu_management/core/policy/policy.h"      // policy_priority_min

int sys_sched_get_priority_min(int policy) {
    return policy_priority_min(policy);
}

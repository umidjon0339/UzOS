/*
 * policy.c - Names, ranges and slices of the scheduling policies, and
 *            the one place a policy is changed.
 *
 * Who runs first is decided in core/priority (priority_rank). This file
 * only knows what each policy means.
 *
 * Uses: power_slice_ticks (core/power), priority_rt_throttled
 *       (core/priority), def_policy_change_allowed (defence
 *       priority_bounds), process_get_by_pid (process_management),
 *       strcmp (lib/string).
 */

#include "modules/cpu_management/core/policy/policy.h"      // this part
#include "modules/cpu_management/core/power/power.h"        // power_slice_ticks
#include "modules/cpu_management/core/priority/priority.h"  // priority_rt_throttled, CPU_SLICE_TICKS
#include "modules/cpu_management/security/priority_abuse/defence/priority_bounds/def_priority_bounds.h"
#include "lib/string/string.h"                              // strcmp

static const struct {
    int number;
    const char *name;
    const char *word;           // what chrt and ps call it
    const char *short_name;     // the ps CLS column
} policies[] = {
    { SCHED_OTHER, "SCHED_OTHER", "other", "TS"  },
    { SCHED_FIFO,  "SCHED_FIFO",  "fifo",  "FF"  },
    { SCHED_RR,    "SCHED_RR",    "rr",    "RR"  },
    { SCHED_BATCH, "SCHED_BATCH", "batch", "B"   },
    { SCHED_IDLE,  "SCHED_IDLE",  "idle",  "IDL" },
};
#define POLICY_COUNT (sizeof(policies) / sizeof(policies[0]))

static int index_of(int policy) {
    for (unsigned i = 0; i < POLICY_COUNT; i++)
        if (policies[i].number == policy)
            return i;
    return -1;
}

int policy_valid(int policy) {
    return index_of(policy) >= 0;
}

int policy_is_realtime(int policy) {
    return policy == SCHED_FIFO || policy == SCHED_RR;
}

int policy_priority_min(int policy) {
    if (!policy_valid(policy))
        return -1;
    return policy_is_realtime(policy) ? RT_PRIORITY_MIN : 0;
}

int policy_priority_max(int policy) {
    if (!policy_valid(policy))
        return -1;
    return policy_is_realtime(policy) ? RT_PRIORITY_MAX : 0;
}

const char *policy_name(int policy) {
    int i = index_of(policy);
    return i >= 0 ? policies[i].name : "?";
}

const char *policy_short(int policy) {
    int i = index_of(policy);
    return i >= 0 ? policies[i].short_name : "?";
}

int policy_find(const char *name) {
    for (unsigned i = 0; i < POLICY_COUNT; i++)
        if (strcmp(name, policies[i].name) == 0 || strcmp(name, policies[i].word) == 0)
            return policies[i].number;
    return -1;
}

uint32_t policy_slice_ticks(const process_t *p) {
    // While real-time is throttled, FIFO and RR are treated like normal ones.
    if (policy_is_realtime(p->policy) && !priority_rt_throttled())
        return p->policy == SCHED_FIFO ? 0 : RR_SLICE_TICKS;
    return power_slice_ticks();
}

int policy_set(const process_t *caller, uint32_t pid, int policy, int rt_priority,
               int reset_on_fork) {
    process_t *target = process_get_by_pid(pid);

    // DEFENCE priority_bounds: a known policy, a priority in its range,
    // not a system process, and real-time only with the right to it.
    if (!def_policy_change_allowed(caller, target, policy, rt_priority))
        return 0;

    target->policy        = policy;
    target->rt_priority   = rt_priority;
    target->reset_on_fork = reset_on_fork;
    return 1;
}

/*
 * def_rt_throttling.c - DEFENCE rt_throttling: real-time work gets most
 *                       of the CPU, but never all of it.
 *
 * Stops:  realtime_scheduling_abuse (security/realtime_scheduling_abuse)
 * Called: the scheduler tick (priority_tick, core/priority); the live
 *         count is priority_rt_throttle_count(). The right to BECOME
 *         real-time is a separate gate (def_policy_change_allowed).
 * Rule:   all SCHED_FIFO/RR tasks together get at most 95% of each
 *         second; past that the kernel runs normal tasks, so the shell
 *         survives even a real-time busy loop.
 * Why:    a real-time task runs before every normal one and need never
 *         sleep; without a throttle it would freeze the machine for good.
 */

#include "modules/cpu_management/security/realtime_scheduling_abuse/defence/rt_throttling/def_rt_throttling.h"
#include "modules/security_management/core/registry/registry.h"
#include "modules/cpu_management/core/priority/priority.h"   // priority_rt_throttle_count

int def_rt_within_budget(uint32_t rt_ticks, uint32_t hz) {
    return rt_ticks <= hz * RT_BUDGET_PERCENT / 100;
}

static int selftest(void) {
    if (!def_rt_within_budget(95, 100)) return 0;    // 95%: still allowed
    if (def_rt_within_budget(96, 100)) return 0;     // past 95%: throttled
    if (!def_rt_within_budget(0, 100)) return 0;     // RT idle: fine
    (void)priority_rt_throttle_count();              // the live counter exists
    return 1;
}

static const defence_t defence = {
    "rt_throttling",
    "real-time tasks get at most 95% of each second; the rest is fair",
    "realtime_scheduling_abuse",
    selftest
};

void def_rt_throttling_register(void) { registry_add_defence(&defence); }

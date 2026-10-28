/*
 * def_policy_restriction.c - DEFENCE policy_restriction: not everyone
 *                            may change how the scheduler treats them.
 *
 * Stops:  scheduling_manipulation (security/scheduling_manipulation)
 * Called: sched_setscheduler / sched_setattr (cpu_management/syscalls),
 *         through def_policy_change_allowed (security/priority_abuse).
 * Rule:   a user may set a NORMAL policy (OTHER/BATCH/IDLE) on its own
 *         process; a REAL-TIME policy needs RLIMIT_RTPRIO; a system
 *         process may not be touched; root may do anything in range.
 * Why:    picking your own policy (say SCHED_FIFO) is a way to jump the
 *         queue ahead of everyone. The gate decides who is allowed to.
 */

#include "modules/cpu_management/security/scheduling_manipulation/defence/policy_restriction/def_policy_restriction.h"
#include "modules/security_management/core/registry/registry.h"
#include "modules/cpu_management/security/priority_abuse/defence/priority_bounds/def_priority_bounds.h"  // def_policy_change_allowed
#include "modules/cpu_management/core/policy/policy.h"                                                    // SCHED_*
#include "modules/cpu_management/core/rlimit/rlimit.h"                                                    // RLIMIT_RTPRIO
#include "modules/process_management/core/process/process.h"                                              // process_t

int def_policy_restriction_ok(void) {
    static process_t user, target;
    user.uid = 1; target.uid = 1; target.is_system = 0;
    // A user picking SCHED_FIFO on itself with no RT budget: refused.
    return def_policy_change_allowed(&user, &target, SCHED_FIFO, 50) == 0;
}

static int selftest(void) {
    static process_t user, target, sys;
    user.uid = 1;
    target.uid = 1; target.is_system = 0;
    sys.uid = 0;    sys.is_system = 1;

    if (def_policy_change_allowed(&user, &target, SCHED_FIFO, 50)) return 0; // no RT budget
    user.rlim_cur[RLIMIT_RTPRIO] = 50;
    if (!def_policy_change_allowed(&user, &target, SCHED_FIFO, 50)) return 0; // now within budget
    if (!def_policy_change_allowed(&user, &target, SCHED_BATCH, 0)) return 0; // normal: fine
    if (def_policy_change_allowed(&user, &sys, SCHED_BATCH, 0)) return 0;     // system: never
    if (!def_policy_change_allowed(0, &target, SCHED_RR, 99)) return 0;       // kernel/root
    return 1;
}

static const defence_t defence = {
    "policy_restriction",
    "a user may not switch to a real-time policy without RLIMIT_RTPRIO",
    "scheduling_manipulation",
    selftest
};

void def_policy_restriction_register(void) { registry_add_defence(&defence); }

/*
 * def_priority_bounds.c - DEFENCE priority_bounds: only sane, permitted priorities.
 *
 * Stops:  priority_out_of_range (security/attack/priority_out_of_range)
 * Called: priority_set_nice() (core/priority) and policy_set()
 *         (core/policy), used by setpriority, sched_setscheduler,
 *         sched_setparam, sched_setattr and the commands nice, renice,
 *         chrt, before anything is changed.
 * Rules for a nice value:
 *         1. NICE_MIN <= nice <= NICE_MAX                      (-20..19)
 *         2. the target exists and is not a system process
 *         3. CAP_PRIORITY (root) may set any nice on any process
 *         4. a user only on its own processes, and only UP (nicer),
 *            or down as far as its RLIMIT_NICE allows (20 - limit)
 * Rules for a policy:
 *         1. a policy UzOS knows, rt_priority inside its range
 *            (1..99 for SCHED_FIFO / SCHED_RR, 0 for the others)
 *         2. the target exists and is not a system process
 *         3. CAP_PRIORITY (root) may set any policy on any process
 *         4. a user only on its own processes; real-time only up to
 *            its RLIMIT_RTPRIO (0 by default: no real-time at all)
 * Why:    the numbers come from a user program.
 *           nice 1000 / rt 1000   rank above everything, forever
 *           nice -20 by a user    jumps ahead of every other user
 *           SCHED_FIFO by a user  a busy loop that only throttling stops
 *         And a user who renices the shell slows it for everybody.
 */

#include "modules/cpu_management/security/priority_abuse/defence/priority_bounds/def_priority_bounds.h"
#include "modules/security_management/core/registry/registry.h"   // registry_add_defence
#include "modules/security_management/core/audit/audit.h"         // audit_log
#include "modules/security_management/core/caps/caps.h"           // caps_has, CAP_PRIORITY
#include "modules/cpu_management/core/priority/priority.h"        // NICE_MIN, NICE_MAX
#include "modules/cpu_management/core/policy/policy.h"            // policy_* , SCHED_*
#include "modules/cpu_management/core/rlimit/rlimit.h"            // RLIMIT_NICE, RLIMIT_RTPRIO

// The nice rule, as a pure function so the selftest can try it.
// nice_limit = the caller's RLIMIT_NICE: it may go down to 20 - nice_limit.
static int nice_allowed(int nice, int target_exists, int target_is_system,
                        uint32_t caller_uid, uint32_t target_uid, int current,
                        uint32_t nice_limit) {
    if (nice < NICE_MIN || nice > NICE_MAX) return 0;               // rule 1
    if (!target_exists || target_is_system) return 0;               // rule 2
    if (caps_has(caller_uid, CAP_PRIORITY)) return 1;               // rule 3
    if (caller_uid != target_uid) return 0;                         // rule 4
    return nice >= current || (nice_limit <= 40 && nice >= 20 - (int)nice_limit);
}

// The policy rule, pure as well. rt_limit = the caller's RLIMIT_RTPRIO.
static int policy_allowed(int policy, int rt_priority, int target_exists,
                          int target_is_system, uint32_t caller_uid,
                          uint32_t target_uid, uint32_t rt_limit) {
    if (!policy_valid(policy)) return 0;                            // rule 1
    if (rt_priority < policy_priority_min(policy) ||
        rt_priority > policy_priority_max(policy)) return 0;
    if (!target_exists || target_is_system) return 0;               // rule 2
    if (caps_has(caller_uid, CAP_PRIORITY)) return 1;               // rule 3
    if (caller_uid != target_uid) return 0;                         // rule 4
    return !policy_is_realtime(policy) || (uint32_t)rt_priority <= rt_limit;
}

int def_nice_change_allowed(const process_t *caller, const process_t *target, int nice) {
    uint32_t caller_uid = caller ? caller->uid : 0;
    uint32_t limit = caller ? caller->rlim_cur[RLIMIT_NICE] : RLIM_INFINITY;

    if (nice_allowed(nice, target != 0, target && target->is_system, caller_uid,
                     target ? target->uid : 0, target ? target->nice : 0, limit))
        return 1;

    if (nice < NICE_MIN || nice > NICE_MAX)
        audit_log("priority_bounds", "nice refused: not a value from -20 to 19");
    else
        audit_log("priority_bounds", "nice refused: system process, not yours, or lower than allowed");
    return 0;
}

int def_policy_change_allowed(const process_t *caller, const process_t *target,
                              int policy, int rt_priority) {
    uint32_t caller_uid = caller ? caller->uid : 0;
    uint32_t limit = caller ? caller->rlim_cur[RLIMIT_RTPRIO] : RLIM_INFINITY;

    if (policy_allowed(policy, rt_priority, target != 0, target && target->is_system,
                       caller_uid, target ? target->uid : 0, limit))
        return 1;

    audit_log("priority_bounds", "policy refused: unknown, out of range, system, not yours, or real-time without the right");
    return 0;
}

static int selftest(void) {
    // nice: value, exists, system, caller uid, target uid, current nice, RLIMIT_NICE
    if (nice_allowed(1000, 1, 0, 0, 0, 0, 0)) return 0;     // too high, even for root
    if (nice_allowed(-500, 1, 0, 0, 0, 0, 0)) return 0;     // too low, even for root
    if (nice_allowed(0, 0, 0, 0, 0, 0, 0)) return 0;        // no such process
    if (nice_allowed(19, 1, 1, 0, 0, 0, 0)) return 0;       // system process, even root
    if (nice_allowed(-20, 1, 0, 1, 1, 0, 0)) return 0;      // user lowering its nice
    if (nice_allowed(10, 1, 0, 1, 2, 0, 0)) return 0;       // user vs someone else's
    if (!nice_allowed(10, 1, 0, 1, 1, 0, 0)) return 0;      // user being nicer: fine
    if (!nice_allowed(-5, 1, 0, 1, 1, 0, 25)) return 0;     // RLIMIT_NICE 25: down to -5
    if (nice_allowed(-6, 1, 0, 1, 1, 0, 25)) return 0;      // ...but not further
    if (!nice_allowed(-20, 1, 0, 0, 1, 0, 0)) return 0;     // root: anything in range

    // policy: policy, rt, exists, system, caller uid, target uid, RLIMIT_RTPRIO
    if (policy_allowed(SCHED_DEADLINE, 0, 1, 0, 0, 0, 0)) return 0;  // not in UzOS
    if (policy_allowed(SCHED_FIFO, 1000, 1, 0, 0, 0, 0)) return 0;   // out of range
    if (policy_allowed(SCHED_OTHER, 5, 1, 0, 0, 0, 0)) return 0;     // OTHER takes 0 only
    if (policy_allowed(SCHED_FIFO, 50, 1, 1, 0, 0, 0)) return 0;     // system process
    if (policy_allowed(SCHED_FIFO, 50, 1, 0, 1, 1, 0)) return 0;     // user, no RTPRIO
    if (!policy_allowed(SCHED_FIFO, 10, 1, 0, 1, 1, 10)) return 0;   // user, RTPRIO 10
    if (!policy_allowed(SCHED_BATCH, 0, 1, 0, 1, 1, 0)) return 0;    // user: batch is fine
    if (!policy_allowed(SCHED_RR, 99, 1, 0, 0, 1, 0)) return 0;      // root: fine
    return 1;
}

static const defence_t defence = {
    "priority_bounds",
    "nice -20..19, rt 1..99; users only nicer, real-time needs RLIMIT_RTPRIO",
    "priority_abuse",
    selftest
};

void def_priority_bounds_register(void) {
    registry_add_defence(&defence);
}

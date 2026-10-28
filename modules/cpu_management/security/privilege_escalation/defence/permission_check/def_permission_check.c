/*
 * def_permission_check.c - DEFENCE permission_check: a user may not act
 *                          with root's CPU rights.
 *
 * Stops:  privilege_escalation (security/privilege_escalation)
 * Called: setpriority / sched_setscheduler / sched_setattr
 *         (cpu_management/syscalls), through def_nice_change_allowed and
 *         def_policy_change_allowed (security/priority_abuse). The uid
 *         to cap for comes from caps_of() (security/caps).
 * Rule:   every privileged CPU action checks the caller's uid and caps.
 *         A user gets only what its own limits allow; root (CAP_PRIORITY)
 *         gets the rest. There is no path that skips the check.
 * Why:    priority_abuse feeds BAD NUMBERS; this is the other half -
 *         IN-RANGE actions a user simply is not allowed to take.
 */

#include "modules/cpu_management/security/privilege_escalation/defence/permission_check/def_permission_check.h"
#include "modules/security_management/core/registry/registry.h"
#include "modules/cpu_management/security/priority_abuse/defence/priority_bounds/def_priority_bounds.h"
#include "modules/cpu_management/core/policy/policy.h"              // SCHED_FIFO
#include "modules/process_management/core/process/process.h"       // process_t

static int user_is_fenced(void) {
    static process_t user, target;
    user.uid = 1; target.uid = 1; target.is_system = 0; target.nice = 0;
    // A user with no budget may neither go greedy on nice nor go real-time.
    return !def_nice_change_allowed(&user, &target, -20) &&
           !def_policy_change_allowed(&user, &target, SCHED_FIFO, 50);
}

static int root_is_allowed(void) {
    static process_t target;
    target.uid = 1; target.is_system = 0; target.nice = 0;
    return def_nice_change_allowed(0, &target, -20) &&          // 0 = the kernel/root
           def_policy_change_allowed(0, &target, SCHED_FIFO, 50);
}

int def_permission_check_ok(void) {
    return user_is_fenced() && root_is_allowed();
}

static int selftest(void) {
    if (!user_is_fenced()) return 0;        // a user is held back
    if (!root_is_allowed()) return 0;       // root still works
    return 1;
}

static const defence_t defence = {
    "permission_check",
    "every privileged CPU action checks uid and caps; no path skips it",
    "privilege_escalation",
    selftest
};

void def_permission_check_register(void) { registry_add_defence(&defence); }

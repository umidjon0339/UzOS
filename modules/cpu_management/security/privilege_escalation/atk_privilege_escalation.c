/*
 * atk_privilege_escalation.c - ATTACK privilege_escalation: act with
 *                              rights you were not given.
 *
 * Breaks:     privilege - user code may not act with kernel/root rights
 * Stopped by: permission_check (defence/permission_check)
 *
 * Privilege escalation is doing something only a more privileged user
 * should. In the CPU module the prize is the scheduler: lower your nice
 * below your budget, or make yourself real-time, and you outrank
 * everyone. Unlike priority_abuse (which sends out-of-range numbers),
 * here the numbers are perfectly legal - the point is that a user is
 * not ALLOWED to use them. Every such call checks uid and caps first.
 *
 * simulate(): name the privileged actions and who may take them.
 * launch():   as a user, try nice -20 and SCHED_FIFO (both refused);
 *             confirm root may do both. 0 findings = the checks hold.
 * cleanup():  nothing.
 *
 * Uses: permission_check (its defence), def_nice_change_allowed /
 *       def_policy_change_allowed (priority_abuse), SCHED_* (policy),
 *       process_t, ui.
 */

#include "modules/security_management/core/registry/registry.h"
#include "modules/cpu_management/security/privilege_escalation/defence/permission_check/def_permission_check.h"
#include "modules/cpu_management/security/priority_abuse/defence/priority_bounds/def_priority_bounds.h"
#include "modules/cpu_management/core/policy/policy.h"
#include "modules/process_management/core/process/process.h"
#include "modules/ui/core/style/ui.h"

static int simulate(void) {
    ui_printf("  privileged CPU actions (root, or a matching rlimit):\n");
    ui_printf("    nice below your RLIMIT_NICE budget  (greedier CPU share)\n");
    ui_printf("    SCHED_FIFO/RR without RLIMIT_RTPRIO  (outrank everyone)\n");
    ui_printf("  a user is refused; root (CAP_PRIORITY) is allowed\n");
    return 1;
}

static int launch(void) {
    static process_t user, target;
    user.uid = 1; target.uid = 1; target.is_system = 0; target.nice = 0;
    int findings = 0;
    if (def_nice_change_allowed(&user, &target, -20)) findings++;         // must refuse
    if (def_policy_change_allowed(&user, &target, SCHED_FIFO, 50)) findings++;
    if (!def_permission_check_ok()) findings++;                           // and root still works
    ui_printf("  user grabs nice -20: %s; SCHED_FIFO: %s\n",
              def_nice_change_allowed(&user, &target, -20) ? "GRANTED" : "refused",
              def_policy_change_allowed(&user, &target, SCHED_FIFO, 50)
              ? "GRANTED" : "refused");
    return findings;
}

static int cleanup(void) { return 0; }

static const attack_t attack = {
    "privilege_escalation",
    "a user takes root-only scheduler rights with legal-looking calls",
    INV_PRIVILEGE, SEV_HIGH,
    simulate, launch, cleanup,
    "permission_check"
};

void atk_privilege_escalation_register(void) { registry_add_attack(&attack); }

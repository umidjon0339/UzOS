/*
 * atk_scheduling_manipulation.c - ATTACK scheduling_manipulation: pick
 *                                 a policy that jumps the queue.
 *
 * Breaks:     liveness - the scheduler's order must not be bought cheap
 * Stopped by: policy_restriction (defence/policy_restriction)
 *
 * The scheduler decides who runs next from each process's policy. A
 * program that may set its OWN policy to SCHED_FIFO puts itself ahead
 * of every normal process at once - no busy loop needed. The gate lets
 * a user pick a normal policy but not a real-time one (unless it has
 * RLIMIT_RTPRIO), and never lets it touch a system process.
 *
 * simulate(): explain how a policy switch jumps the queue.
 * launch():   as a user with no RT budget, try SCHED_FIFO on itself
 *             (refused) and a normal policy (allowed). 0 = gate holds.
 * cleanup():  nothing.
 *
 * Uses: policy_restriction (its defence), def_policy_change_allowed
 *       (priority_abuse), SCHED_* (core/policy), process_t, ui.
 */

#include "modules/security_management/core/registry/registry.h"
#include "modules/cpu_management/security/scheduling_manipulation/defence/policy_restriction/def_policy_restriction.h"
#include "modules/cpu_management/security/priority_abuse/defence/priority_bounds/def_priority_bounds.h"
#include "modules/cpu_management/core/policy/policy.h"
#include "modules/process_management/core/process/process.h"
#include "modules/ui/core/style/ui.h"

static int simulate(void) {
    ui_printf("  SCHED_FIFO runs before every normal (SCHED_OTHER) task\n");
    ui_printf("  so switching your own policy to FIFO jumps the queue\n");
    ui_printf("  a user needs RLIMIT_RTPRIO for that; else it is refused\n");
    return 1;
}

static int launch(void) {
    static process_t user, target;
    user.uid = 1; target.uid = 1; target.is_system = 0;
    int findings = 0;
    if (def_policy_change_allowed(&user, &target, SCHED_FIFO, 50)) findings++; // must refuse
    if (!def_policy_change_allowed(&user, &target, SCHED_BATCH, 0)) findings++; // normal ok
    ui_printf("  user with no RT budget asks for SCHED_FIFO: %s\n",
              def_policy_change_allowed(&user, &target, SCHED_FIFO, 50)
              ? "GRANTED" : "refused");
    return findings;
}

static int cleanup(void) { return 0; }

static const attack_t attack = {
    "scheduling_manipulation",
    "switch your own policy to real-time to jump ahead of everyone",
    INV_LIVENESS, SEV_MEDIUM,
    simulate, launch, cleanup,
    "policy_restriction"
};

void atk_scheduling_manipulation_register(void) { registry_add_attack(&attack); }

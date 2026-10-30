/*
 * atk_priority_inversion.c - ATTACK priority_inversion (concept).
 *
 * Breaks:     liveness - a high-priority process must not wait for a medium one
 * Stopped by: priority_inheritance (defence/priority_inheritance)
 *
 * LOW (priority 1) takes a lock. HIGH (7) needs it and waits. MEDIUM
 * (4), which needs no lock, is runnable and outranks LOW, so LOW never
 * runs, never releases the lock - and HIGH waits for MEDIUM. A user can
 * cause this on purpose: hold a lock a privileged daemon needs, then
 * keep MEDIUM-priority work busy.
 *
 * simulate(): the three processes.
 * launch():   CONCEPT: UzOS user programs share no locks (no futex), so
 *             the inversion cannot be built here. It shows what LOW runs
 *             at without and with the rule. findings = 0.
 * cleanup():  nothing.
 *
 * Uses: def_pi_priority (its defence), ui.
 */

#include "modules/security_management/core/registry/registry.h"   // attack_t, registry_add_attack
#include "modules/process_management/security/priority_inversion/defence/priority_inheritance/def_priority_inheritance.h"  // def_pi_priority
#include "modules/ui/core/style/ui.h"                             // ui_printf

#define LOW    1
#define MEDIUM 4
#define HIGH   7

static int simulate(void) {
    ui_printf("  LOW (%d) holds a lock; HIGH (%d) waits for it; MEDIUM (%d) runs\n", LOW, HIGH, MEDIUM);
    ui_printf("  LOW never gets the CPU, so HIGH waits for MEDIUM\n");
    return 1;
}

static int launch(void) {
    ui_printf("  concept: UzOS programs share no locks (no futex) to block on\n");
    ui_printf("  LOW runs at %d without the rule: MEDIUM (%d) wins, HIGH starves\n", LOW, MEDIUM);
    ui_printf("  LOW runs at %d with priority inheritance: it beats MEDIUM, lets go\n",
              def_pi_priority(LOW, HIGH));
    return 0;
}

static int cleanup(void) { return 0; }

static const attack_t attack = {
    "priority_inversion",
    "a low-priority lock holder makes a high-priority waiter wait (concept)",
    INV_LIVENESS, SEV_INFO,
    simulate, launch, cleanup,
    "priority_inheritance"
};

void atk_priority_inversion_register(void) { registry_add_attack(&attack); }

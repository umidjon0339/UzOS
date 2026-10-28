/*
 * atk_denial_of_service.c - ATTACK denial_of_service: hog the CPU so
 *                           real work never runs.
 *
 * Breaks:     liveness - every ready process must get the CPU in the end
 * Stopped by: fair_scheduling (defence/fair_scheduling)
 *
 * On one machine, "denial of service" means keeping the CPU so busy
 * that the things people need - the shell, a server - never get a turn.
 * A strict priority scheduler would let a flood of high-priority work
 * do exactly that. Aging plus round robin put a ceiling on the wait:
 * whatever has waited longest is eventually served.
 *
 * simulate(): show that even the lowest process reaches the top after
 *             enough waiting.
 * launch():   confirm the real aging rule lifts a starved process to
 *             the top level (0 findings = service is not denied).
 * cleanup():  nothing.
 *
 * Uses: fair_scheduling (its defence), def_aging_effective (aging),
 *       PRIORITY_* (core/priority), ui_printf (ui).
 */

#include "modules/security_management/core/registry/registry.h"
#include "modules/cpu_management/security/denial_of_service/defence/fair_scheduling/def_fair_scheduling.h"
#include "modules/cpu_management/security/starvation/defence/aging/def_aging.h"
#include "modules/cpu_management/core/priority/priority.h"
#include "modules/ui/core/style/ui.h"

static int simulate(void) {
    ui_printf("  a flood of top-priority work would hog the CPU\n");
    ui_printf("  aging lifts a level-%d process to level %d after %u ms\n",
              PRIORITY_MIN, PRIORITY_MAX, (unsigned)(6 * AGING_TICKS) * 10u);
    ui_printf("  then the round robin gives it a turn: service resumes\n");
    return 1;
}

static int launch(void) {
    int findings = def_fair_scheduling_ok() ? 0 : 1;
    ui_printf("  a starved process is eventually served: %s\n",
              findings ? "NO (denied!)" : "yes");
    return findings;
}

static int cleanup(void) { return 0; }

static const attack_t attack = {
    "denial_of_service",
    "monopolise the CPU so the shell and servers never run",
    INV_LIVENESS, SEV_HIGH,
    simulate, launch, cleanup,
    "fair_scheduling"
};

void atk_denial_of_service_register(void) { registry_add_attack(&attack); }

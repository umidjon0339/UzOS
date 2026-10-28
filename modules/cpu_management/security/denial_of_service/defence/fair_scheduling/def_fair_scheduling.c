/*
 * def_fair_scheduling.c - DEFENCE fair_scheduling: no one is denied the
 *                         CPU for good.
 *
 * Stops:  denial_of_service (security/denial_of_service)
 * Called: the scheduler, on every decision, through aging
 *         (priority_effective -> def_aging_effective, core/priority).
 * Rule:   a ready process rises one level per 100 ms of waiting until
 *         it ties the busiest task and the round robin gives it a turn.
 *         So even a flood of high-priority work cannot deny service to
 *         the shell for ever.
 * Why:    "denial of service" on one machine means hogging the CPU so
 *         real work never runs. Aging + round robin put a ceiling on
 *         how long anything can be kept waiting.
 */

#include "modules/cpu_management/security/denial_of_service/defence/fair_scheduling/def_fair_scheduling.h"
#include "modules/security_management/core/registry/registry.h"
#include "modules/cpu_management/security/starvation/defence/aging/def_aging.h"  // def_aging_effective, AGING_TICKS
#include "modules/cpu_management/core/priority/priority.h"                        // PRIORITY_MIN/MAX

int def_fair_scheduling_ok(void) {
    // The lowest process, after enough waiting, reaches the top level.
    return def_aging_effective(PRIORITY_MIN, 6 * AGING_TICKS) == PRIORITY_MAX;
}

static int selftest(void) {
    if (def_aging_effective(PRIORITY_MIN, 0) != PRIORITY_MIN) return 0;   // no wait yet
    if (!def_fair_scheduling_ok()) return 0;                              // waiting wins
    if (def_aging_effective(PRIORITY_MIN, 0xFFFFFFFF) > PRIORITY_MAX) return 0;
    return 1;
}

static const defence_t defence = {
    "fair_scheduling",
    "aging + round robin: a waiting process is always served in the end",
    "denial_of_service",
    selftest
};

void def_fair_scheduling_register(void) { registry_add_defence(&defence); }

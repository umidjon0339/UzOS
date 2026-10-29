/*
 * def_service_limits.c - DEFENCE service_limits (points at the real caps).
 *
 * Stops:  process_dos (security/process_dos)
 * Called: the limits that already do the work - slot_quota in
 *         process_create (process count) and the CPU quotas /
 *         RLIMIT_CPU in cpu_management (CPU time).
 * Rule:   no single user may take all the process slots or all the CPU,
 *         so a flood from one account cannot deny service to the rest.
 * Why:    "process-based DoS" is the umbrella over fork bombs, process-
 *         table exhaustion and CPU hogs. Each already has its own live
 *         defence; this pair names the umbrella and points at them.
 */

#include "def_service_limits.h"
#include "registry.h"

int def_service_limits_apply(void) { return 1; }   // slot_quota + CPU quotas

static int selftest(void) { return def_service_limits_apply(); }

static const defence_t defence = {
    "service_limits",
    "per-user process and CPU caps keep one account from starving the rest",
    "process_dos",
    selftest
};

void def_service_limits_register(void) { registry_add_defence(&defence); }

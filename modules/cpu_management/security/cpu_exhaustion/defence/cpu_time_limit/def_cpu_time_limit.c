/*
 * def_cpu_time_limit.c - DEFENCE cpu_time_limit: RLIMIT_CPU kills a hog.
 *
 * Stops:  cpu_exhaustion (security/cpu_exhaustion)
 * Called: the timer tick, through rlimit_check_cpu() (core/rlimit),
 *         once a second for every process.
 * Rule:   a process over its soft RLIMIT_CPU (seconds) is killed;
 *         RLIM_INFINITY means no limit. A child inherits the limit, so
 *         `ulimit -t N` in the shell caps every command started after.
 * Why:    a runaway or hostile loop would hold the CPU for good; the
 *         limit makes it stop on its own, without anyone watching.
 */

#include "modules/cpu_management/security/cpu_exhaustion/defence/cpu_time_limit/def_cpu_time_limit.h"
#include "modules/security_management/core/registry/registry.h"   // defence_t
#include "modules/cpu_management/core/rlimit/rlimit.h"            // RLIM_INFINITY

int def_cpu_over_limit(uint32_t used_sec, uint32_t limit_sec) {
    if (limit_sec == RLIM_INFINITY)
        return 0;                       // no limit set: never over
    return used_sec > limit_sec;
}

static int selftest(void) {
    if (!def_cpu_over_limit(2, 1)) return 0;                 // over: must kill
    if (def_cpu_over_limit(1, 1)) return 0;                  // at the limit: not yet
    if (def_cpu_over_limit(0, 1)) return 0;                  // used nothing
    if (def_cpu_over_limit(9999, RLIM_INFINITY)) return 0;   // unlimited
    if (!rlimit_supported(RLIMIT_CPU)) return 0;             // the kernel enforces it
    return 1;
}

static const defence_t defence = {
    "cpu_time_limit",
    "RLIMIT_CPU (ulimit -t): over the soft limit, the timer kills it",
    "cpu_exhaustion",
    selftest
};

void def_cpu_time_limit_register(void) { registry_add_defence(&defence); }

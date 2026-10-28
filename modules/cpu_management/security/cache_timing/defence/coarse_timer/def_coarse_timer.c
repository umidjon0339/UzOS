/*
 * def_coarse_timer.c - DEFENCE coarse_timer: deny ring 3 a fine clock.
 *
 * Stops:  cache_timing (security/cache_timing)
 * Called: the same CR4.TSD trap as side_channel's tsc_restrict: rdtsc
 *         in ring 3 faults (def_tsc_restrict_enable, core/tsc). Without
 *         rdtsc, ring 3's finest clock is the timer tick (10 ms).
 * Rule:   a timer whose smallest step is far larger than the cache
 *         hit/miss gap cannot tell a hit from a miss, so cache timing
 *         learns nothing.
 * Why:    cache attacks (Flush+Reload, Prime+Probe) all rest on a fine
 *         clock. Take the clock away and the side channel goes quiet.
 */

#include "modules/cpu_management/security/cache_timing/defence/coarse_timer/def_coarse_timer.h"
#include "modules/security_management/core/registry/registry.h"

int def_too_coarse_to_resolve(uint32_t gap_cycles, uint32_t resolution_cycles) {
    return resolution_cycles > gap_cycles;
}

static int selftest(void) {
    // ~200-cycle cache gap vs a 10 ms tick (~10M cycles at 1 GHz): blind.
    if (!def_too_coarse_to_resolve(200, 10000000)) return 0;
    // A fine rdtsc clock (1-cycle steps) WOULD see it: that is why ring 3
    // rdtsc is trapped.
    if (def_too_coarse_to_resolve(200, 1)) return 0;
    return 1;
}

static const defence_t defence = {
    "coarse_timer",
    "ring 3 rdtsc is trapped; the tick is too coarse to time a cache",
    "cache_timing",
    selftest
};

void def_coarse_timer_register(void) { registry_add_defence(&defence); }

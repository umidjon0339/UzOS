/*
 * atk_cache_timing.c - ATTACK cache_timing (ILLUSTRATION): read secrets
 *                      from how long a memory access takes.
 *
 * Breaks:     disclosure - access time must not reveal what is cached
 * Stopped by: coarse_timer (defence/coarse_timer)
 *
 * A cached byte loads in a few dozen cycles; one from RAM takes a few
 * hundred. By timing accesses an attacker learns which addresses a
 * victim touched - and so, often, a secret (a key bit, a branch taken).
 * Every such attack needs a fine clock. UzOS traps ring 3 rdtsc (same
 * as side_channel), leaving only the 10 ms tick, far too coarse to see
 * a cache hit. This is the measurement half of the side_channel family.
 *
 * simulate(): the hit-vs-miss gap, and the clock an attacker would need.
 * launch():   confirm the tick is too coarse to resolve the gap.
 * cleanup():  nothing.
 *
 * Uses: coarse_timer (its defence), ui_printf (ui).
 */

#include "modules/security_management/core/registry/registry.h"
#include "modules/cpu_management/security/cache_timing/defence/coarse_timer/def_coarse_timer.h"
#include "modules/ui/core/style/ui.h"

#define CACHE_GAP_CYCLES   200
#define TICK_CYCLES        10000000u   // ~10 ms at 1 GHz

static int simulate(void) {
    ui_printf("  cache hit ~50 cycles, miss ~250: a ~%d-cycle tell\n",
              CACHE_GAP_CYCLES);
    ui_printf("  timing accesses reveals which addresses were touched\n");
    ui_printf("  it needs a fine clock; ring 3 rdtsc is trapped in UzOS\n");
    return 1;
}

static int launch(void) {
    int blind = def_too_coarse_to_resolve(CACHE_GAP_CYCLES, TICK_CYCLES);
    ui_printf("  tick too coarse to see a cache hit: %s\n",
              blind ? "yes (blind)" : "NO");
    return blind ? 0 : 1;
}

static int cleanup(void) { return 0; }

static const attack_t attack = {
    "cache_timing",
    "read secrets from cache hit/miss timing (illustration)",
    INV_DISCLOSURE, SEV_INFO,
    simulate, launch, cleanup,
    "coarse_timer"
};

void atk_cache_timing_register(void) { registry_add_attack(&attack); }

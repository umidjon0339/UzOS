/*
 * atk_cpu_exhaustion.c - ATTACK cpu_exhaustion: burn every CPU second.
 *
 * Breaks:     resource - CPU time is finite; one task must not take all
 * Stopped by: cpu_time_limit (defence/cpu_time_limit)
 *
 * A program that never sleeps uses one whole CPU second every second.
 * A runaway or hostile task that does this keeps the CPU from every
 * other process. RLIMIT_CPU (ulimit -t) says "you get N CPU seconds,
 * then the kernel kills you", so a runaway stops on its own.
 *
 * simulate(): the arithmetic - ticks per second, when the kill lands.
 * launch():   ask the real rule if 2 s used is over a 1 s limit (yes),
 *             at 1 s (no), under no limit (no). 0 findings = it holds.
 * cleanup():  nothing (launch only reads the rule).
 *
 * Uses: cpu_time_limit (its defence), TIMER_HZ (device/timer),
 *       RLIM_INFINITY (core/rlimit), ui_printf (ui).
 */

#include "modules/security_management/core/registry/registry.h"   // attack_t
#include "modules/cpu_management/security/cpu_exhaustion/defence/cpu_time_limit/def_cpu_time_limit.h"
#include "modules/cpu_management/core/rlimit/rlimit.h"            // RLIM_INFINITY
#include "modules/device_management/timer/timer.h"               // TIMER_HZ
#include "modules/ui/core/style/ui.h"                            // ui_printf

static int simulate(void) {
    ui_printf("  a busy loop uses 1 CPU second every second (%u ticks)\n",
              (unsigned)TIMER_HZ);
    ui_printf("  with `ulimit -t 1` the timer kills it after about 1 s\n");
    ui_printf("  with no limit it would hold the CPU for good\n");
    return 1;
}

static int launch(void) {
    int findings = 0;
    if (!def_cpu_over_limit(2, 1)) findings++;              // 2 s vs 1 s: must kill
    if (def_cpu_over_limit(1, 1)) findings++;              // at the limit: not yet
    if (def_cpu_over_limit(9, RLIM_INFINITY)) findings++; // unlimited: never
    ui_printf("  used 2 s under a 1 s limit: %s\n",
              def_cpu_over_limit(2, 1) ? "killed" : "ALLOWED");
    return findings;                                       // 0 = the limit holds
}

static int cleanup(void) { return 0; }

static const attack_t attack = {
    "cpu_exhaustion",
    "a never-sleeping loop takes a whole CPU for itself",
    INV_RESOURCE, SEV_HIGH,
    simulate, launch, cleanup,
    "cpu_time_limit"
};

void atk_cpu_exhaustion_register(void) { registry_add_attack(&attack); }

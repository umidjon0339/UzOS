/*
 * atk_infinite_loop.c - ATTACK infinite_loop: run forever, do nothing.
 *
 * Breaks:     resource - CPU cycles spent on a stuck loop are wasted
 * Stopped by: cpu_watchdog (defence/cpu_watchdog)
 *
 * `while (1);` is the simplest bug and the simplest attack: the process
 * makes no progress but never gives the CPU back. A kernel cannot read
 * the program's mind to tell "useful" from "stuck", so it watches the
 * one thing it can measure - CPU time - and stops a process that spends
 * more than its budget (RLIMIT_CPU).
 *
 * simulate(): explain why "detect no progress" is not something a
 *             kernel can do, and what it does instead.
 * launch():   ask the real rule if 5 s used is past a 2 s budget (yes).
 * cleanup():  nothing.
 *
 * Uses: cpu_watchdog (its defence), ui_printf (ui), RLIM_INFINITY (rlimit).
 */

#include "modules/security_management/core/registry/registry.h"
#include "modules/cpu_management/security/infinite_loop/defence/cpu_watchdog/def_cpu_watchdog.h"
#include "modules/cpu_management/core/rlimit/rlimit.h"
#include "modules/ui/core/style/ui.h"

static int simulate(void) {
    ui_printf("  the loop never returns and never sleeps\n");
    ui_printf("  a kernel cannot tell a stuck loop from useful work\n");
    ui_printf("  so it kills whatever spends more CPU than its budget\n");
    return 1;
}

static int launch(void) {
    int findings = 0;
    if (!def_watchdog_over(5, 2)) findings++;                 // past the budget: stop
    if (def_watchdog_over(100, RLIM_INFINITY)) findings++;    // no budget: runs on
    ui_printf("  spun 5 s on a 2 s budget: %s\n",
              def_watchdog_over(5, 2) ? "stopped" : "STILL RUNNING");
    return findings;
}

static int cleanup(void) { return 0; }

static const attack_t attack = {
    "infinite_loop",
    "a loop that never returns and never yields the CPU",
    INV_RESOURCE, SEV_MEDIUM,
    simulate, launch, cleanup,
    "cpu_watchdog"
};

void atk_infinite_loop_register(void) { registry_add_attack(&attack); }

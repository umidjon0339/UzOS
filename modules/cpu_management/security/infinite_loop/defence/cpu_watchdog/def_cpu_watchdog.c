/*
 * def_cpu_watchdog.c - DEFENCE cpu_watchdog: a stuck process is stopped.
 *
 * Stops:  infinite_loop (security/infinite_loop)
 * Called: the timer tick, through rlimit_check_cpu() (core/rlimit).
 * Rule:   a kernel cannot tell a useful loop from a stuck one, so the
 *         watchdog is a time budget: over the soft RLIMIT_CPU, the
 *         process is killed. No human has to notice it is stuck.
 * Why:    `while (1);` by mistake would otherwise freeze that process
 *         (and, at a high priority, the machine) until a reboot.
 */

#include "modules/cpu_management/security/infinite_loop/defence/cpu_watchdog/def_cpu_watchdog.h"
#include "modules/security_management/core/registry/registry.h"
#include "modules/cpu_management/core/rlimit/rlimit.h"

int def_watchdog_over(uint32_t used_sec, uint32_t limit_sec) {
    if (limit_sec == RLIM_INFINITY)
        return 0;
    return used_sec > limit_sec;
}

static int selftest(void) {
    if (!def_watchdog_over(5, 2)) return 0;                  // stuck past budget
    if (def_watchdog_over(2, 2)) return 0;                   // still inside it
    if (def_watchdog_over(100, RLIM_INFINITY)) return 0;     // no budget set
    if (!rlimit_supported(RLIMIT_CPU)) return 0;             // the timer enforces it
    return 1;
}

static const defence_t defence = {
    "cpu_watchdog",
    "no progress test is impossible; a CPU-time budget stops the spin",
    "infinite_loop",
    selftest
};

void def_cpu_watchdog_register(void) { registry_add_defence(&defence); }

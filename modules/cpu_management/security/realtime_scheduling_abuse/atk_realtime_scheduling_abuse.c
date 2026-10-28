/*
 * atk_realtime_scheduling_abuse.c - ATTACK realtime_scheduling_abuse:
 *                                   a real-time busy loop to own the CPU.
 *
 * Breaks:     liveness - real-time work must not freeze the machine
 * Stopped by: rt_throttling (defence/rt_throttling)
 *
 * SCHED_FIFO/RR tasks run before every normal task and a FIFO task need
 * never yield. A real-time busy loop would therefore take the CPU for
 * good - worse than a normal hog, which aging can still rescue others
 * from. Two things stop it: you need RLIMIT_RTPRIO to become real-time
 * at all (policy_restriction), and even then RT work is capped at 95%
 * of each second (rt_throttling), so normal tasks still run.
 *
 * simulate(): the budget arithmetic at 100 Hz.
 * launch():   ask the real rule whether 96 of 100 ticks is within the
 *             budget (no). 0 findings = the throttle holds.
 * cleanup():  nothing.
 *
 * Uses: rt_throttling (its defence), TIMER_HZ (device/timer), ui.
 */

#include "modules/security_management/core/registry/registry.h"
#include "modules/cpu_management/security/realtime_scheduling_abuse/defence/rt_throttling/def_rt_throttling.h"
#include "modules/device_management/timer/timer.h"
#include "modules/ui/core/style/ui.h"

static int simulate(void) {
    uint32_t budget = (uint32_t)TIMER_HZ * RT_BUDGET_PERCENT / 100;
    ui_printf("  a SCHED_FIFO loop never yields and outranks normal work\n");
    ui_printf("  RT budget: %u of %u ticks per second (%d%%)\n",
              (unsigned)budget, (unsigned)TIMER_HZ, RT_BUDGET_PERCENT);
    ui_printf("  past that the kernel runs normal tasks, so the shell lives\n");
    return 1;
}

static int launch(void) {
    int findings = 0;
    if (def_rt_within_budget(TIMER_HZ, TIMER_HZ)) findings++;       // 100%: must throttle
    if (!def_rt_within_budget(0, TIMER_HZ)) findings++;            // idle RT: fine
    ui_printf("  real-time loop wants 100%% of the second: %s\n",
              def_rt_within_budget(TIMER_HZ, TIMER_HZ) ? "GRANTED" : "throttled");
    return findings;
}

static int cleanup(void) { return 0; }

static const attack_t attack = {
    "realtime_scheduling_abuse",
    "a SCHED_FIFO busy loop outranks everything and never yields",
    INV_LIVENESS, SEV_HIGH,
    simulate, launch, cleanup,
    "rt_throttling"
};

void atk_realtime_scheduling_abuse_register(void) { registry_add_attack(&attack); }

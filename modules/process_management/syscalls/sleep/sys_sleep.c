/*
 * sys_sleep.c - sleep(): stop running for 'ms' milliseconds.
 *
 * The process is marked SLEEPING with a tick count. The scheduler skips
 * it, and every timer tick counts it down; at 0 it becomes READY again.
 * Sleeping costs no CPU time, unlike a busy loop.
 *
 * sys_sleep_interruptible() is the same, except that signal_send()
 * (core/signal) may wake the process early: Ctrl+C ends `sleep 30`.
 *
 * Uses: the scheduler (current process, yield), def_sleep_ms_ticks
 *       (security/sleep_overflow/defence/sleep_bounds), cpu_irq_save
 *       (arch/x86/cpu), SIGINT bits (core/signal).
 */

#include "modules/process_management/syscalls/sleep/sys_sleep.h"                                       // this file
#include "modules/process_management/core/signal/signal.h"                                             // signal_deliverable
#include "modules/process_management/core/scheduler/scheduler.h"                                       // scheduler_get_current_process, scheduler_yield
#include "modules/process_management/security/sleep_overflow/defence/sleep_bounds/def_sleep_bounds.h"  // def_sleep_ms_ticks
#include "arch/x86/cpu/cpu.h"                                                                          // cpu_irq_save, cpu_irq_restore

// At least one tick, rounded up: sleep(1) still sleeps. DEFENCE
// sleep_bounds: no overflow for huge ms (the old "(ms + 9) / 10" gave 0).
static uint32_t ms_to_ticks(uint32_t ms) {
    uint32_t ticks = def_sleep_ms_ticks(ms);
    return ticks ? ticks : 1;
}

void sys_sleep(uint32_t ms) {
    process_t *self = scheduler_get_current_process();
    if (!self)
        return;

    self->sleep_ticks = ms_to_ticks(ms);
    self->state = PROCESS_STATE_SLEEPING;

    // Normally one yield is enough. Looping also covers the case where
    // there was nobody to switch to, so we never return early.
    while (self->state == PROCESS_STATE_SLEEPING)
        scheduler_yield();
}

int sys_sleep_interruptible(uint32_t ms) {
    process_t *self = scheduler_get_current_process();
    if (!self)
        return 1;

    // Check for a signal and go to sleep in one step, with interrupts off:
    // a Ctrl+C in between would otherwise find us awake (nothing to wake),
    // and we would then sleep the whole time anyway.
    uint32_t flags = cpu_irq_save();
    if (signal_deliverable(self)) {
        cpu_irq_restore(flags);
        return 0;                       // already interrupted: do not sleep
    }
    self->sleep_ticks = ms_to_ticks(ms);
    self->sleep_interruptible = 1;
    self->state = PROCESS_STATE_SLEEPING;
    cpu_irq_restore(flags);

    while (self->state == PROCESS_STATE_SLEEPING)
        scheduler_yield();

    self->sleep_interruptible = 0;
    return signal_deliverable(self) == 0;
}

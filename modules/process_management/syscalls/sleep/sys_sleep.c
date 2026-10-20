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
 * Uses: the scheduler (current process, yield), TIMER_HZ (device_management
 *       timer): at 100 ticks per second, one tick = 10 ms, cpu_irq_save
 *       (arch/x86/cpu), SIGINT bits (core/signal).
 */

#include "sys_sleep.h"
#include "scheduler.h"
#include "timer.h"
#include "cpu.h"

#define MS_PER_TICK (1000 / TIMER_HZ)

// At least one tick, rounded up: sleep(1) still sleeps.
static uint32_t ms_to_ticks(uint32_t ms) {
    uint32_t ticks = (ms + MS_PER_TICK - 1) / MS_PER_TICK;
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
    if (self->signals) {
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
    return self->signals == 0;
}

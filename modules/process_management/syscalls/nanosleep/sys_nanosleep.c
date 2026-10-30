/*
 * sys_nanosleep.c - nanosleep(req, rem): sleep this long.
 *
 * The time is rounded UP to whole timer ticks (10 ms): asking for 1 ns
 * sleeps one tick, never less than asked. A signal that is not blocked
 * ends the sleep early: then it returns -1 (EINTR) and *rem says how
 * much was left, so the program can sleep again for the rest.
 * Returns 0, or -1 for a bad time (sleep_bounds) or a signal.
 *
 * Uses: scheduler_get_current_process / scheduler_yield (core/scheduler),
 *       signal_deliverable (core/signal), timer_ticks / TIMER_HZ
 *       (device_management), cpu_irq_save, def_sleep_time_ticks
 *       (security/sleep_overflow/defence/sleep_bounds).
 */
#include "modules/process_management/syscalls/nanosleep/sys_nanosleep.h"  // this file
#include "modules/process_management/core/scheduler/scheduler.h"          // scheduler_get_current_process, scheduler_yield
#include "modules/process_management/core/signal/signal.h"                // signal_deliverable
#include "modules/device_management/timer/timer.h"                        // timer_ticks, TIMER_HZ
#include "arch/x86/cpu/cpu.h"                                             // cpu_irq_save, cpu_irq_restore
#include "modules/process_management/security/sleep_overflow/defence/sleep_bounds/def_sleep_bounds.h"  // def_sleep_time_ticks

// Sleep 'ticks', woken early by a signal. Returns the ticks left (0 = all slept).
static uint32_t sleep_ticks(process_t *self, uint32_t ticks) {
    uint32_t end = timer_ticks + ticks;
    uint32_t flags = cpu_irq_save();            // check and sleep in one step
    if (signal_deliverable(self)) {
        cpu_irq_restore(flags);
        return ticks;
    }
    self->sleep_ticks = ticks;
    self->sleep_interruptible = 1;
    self->state = PROCESS_STATE_SLEEPING;
    cpu_irq_restore(flags);

    while (self->state == PROCESS_STATE_SLEEPING || self->state == PROCESS_STATE_STOPPED)
        scheduler_yield();
    self->sleep_interruptible = 0;
    int32_t left = (int32_t)(end - timer_ticks);
    return left > 0 && signal_deliverable(self) ? (uint32_t)left : 0;
}

int sys_nanosleep(const nanosleep_timespec_t *req, nanosleep_timespec_t *rem) {
    process_t *self = scheduler_get_current_process();
    uint32_t ticks;
    if (!self || !req)
        return -1;
    // DEFENCE sleep_bounds: a valid time, converted without overflow.
    if (!def_sleep_time_ticks(req->sec, req->nsec, 1000000000u, &ticks))
        return -1;                              // EINVAL

    uint32_t left = ticks ? sleep_ticks(self, ticks) : 0;
    if (rem) {
        rem->sec  = (int32_t)(left / TIMER_HZ);
        rem->nsec = (int32_t)(left % TIMER_HZ) * (1000000000 / TIMER_HZ);
    }
    return left ? -1 : 0;                       // -1: EINTR
}

/*
 * sys_clock_nanosleep.c - clock_nanosleep(clock, flags, req, rem).
 *
 * nanosleep() with a choice:
 *   clock   CLOCK_MONOTONIC (since boot) or CLOCK_REALTIME (the date)
 *   flags   0: sleep for 'req'     TIMER_ABSTIME: sleep UNTIL 'req'
 *           ("wake me at 12:00:00" does not drift, however long the
 *           program took between two sleeps)
 * A moment in the past returns at once. Like Linux, it returns the error
 * number itself: 0, CNS_EINVAL (bad clock / time), CNS_EINTR (a signal;
 * *rem = what was left, for a relative sleep).
 *
 * Uses: scheduler_get_current_process / scheduler_yield (core/scheduler),
 *       signal_deliverable (core/signal), timer_ticks / TIMER_HZ, sys_time
 *       (device_management), cpu_irq_save, def_sleep_time_ticks
 *       (security/sleep_overflow/defence/sleep_bounds).
 */
#include "modules/process_management/syscalls/clock_nanosleep/sys_clock_nanosleep.h"  // this file
#include "modules/process_management/core/scheduler/scheduler.h"                      // scheduler_get_current_process, scheduler_yield
#include "modules/process_management/core/signal/signal.h"                            // signal_deliverable
#include "modules/device_management/timer/timer.h"                                    // timer_ticks, TIMER_HZ
#include "modules/device_management/syscalls/time/sys_time.h"                         // sys_time
#include "arch/x86/cpu/cpu.h"                                                         // cpu_irq_save, cpu_irq_restore
#include "modules/process_management/security/sleep_overflow/defence/sleep_bounds/def_sleep_bounds.h"  // def_sleep_time_ticks

static uint32_t sleep_ticks(process_t *self, uint32_t ticks) {
    uint32_t end = timer_ticks + ticks;
    uint32_t flags = cpu_irq_save();
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

int sys_clock_nanosleep(int clock, int flags, const clock_timespec_t *req, clock_timespec_t *rem) {
    process_t *self = scheduler_get_current_process();
    uint32_t ticks;
    if (!self || !req || (clock != CLOCK_REALTIME && clock != CLOCK_MONOTONIC) || (flags & ~TIMER_ABSTIME))
        return CNS_EINVAL;
    // A moment on the date clock: first how many seconds away it is
    // (seconds since 1970, times 100 ticks, would not fit 32 bits).
    int32_t sec = req->sec;
    if ((flags & TIMER_ABSTIME) && clock == CLOCK_REALTIME) {
        sec -= (int32_t)sys_time();
        if (sec < 0)
            return 0;                           // already past
    }
    // DEFENCE sleep_bounds: a valid time, converted without overflow.
    if (!def_sleep_time_ticks(sec, req->nsec, 1000000000u, &ticks))
        return CNS_EINVAL;
    if ((flags & TIMER_ABSTIME) && clock == CLOCK_MONOTONIC)   // a moment since boot
        ticks = (int32_t)(ticks - timer_ticks) > 0 ? ticks - timer_ticks : 0;
    uint32_t left = ticks ? sleep_ticks(self, ticks) : 0;
    if (rem && !(flags & TIMER_ABSTIME)) {
        rem->sec  = (int32_t)(left / TIMER_HZ);
        rem->nsec = (int32_t)(left % TIMER_HZ) * (1000000000 / TIMER_HZ);
    }
    return left ? CNS_EINTR : 0;
}

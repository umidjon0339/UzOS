/*
 * sys_setitimer.c - setitimer(which, new, old): arm or disarm a timer.
 *
 *   which   ITIMER_REAL     wall clock      -> SIGALRM
 *           ITIMER_VIRTUAL  my ring 3 CPU   -> SIGVTALRM
 *           ITIMER_PROF     all my CPU      -> SIGPROF
 *   new     value = when it first fires (0 disarms), interval = then
 *           every so often (0 = once)
 *   old     what it was set to before (time left, interval)
 * Times are rounded up to ticks (10 ms) by sleep_bounds; a periodic
 * timer needs a free place under timer_limits. alarm() shares
 * ITIMER_REAL. signal_tick() (core/signal) runs them. Returns 0 or -1.
 *
 * Uses: scheduler_get_current_process (core/scheduler), ITIMER_* (core/signal),
 *       TIMER_HZ (device_management), cpu_irq_save, def_sleep_time_ticks
 *       (security/sleep_overflow/defence/sleep_bounds), def_itimer_allowed
 *       (security/timer_flood/defence/timer_limits).
 */
#include "modules/process_management/syscalls/setitimer/sys_setitimer.h"  // this file
#include "modules/process_management/core/scheduler/scheduler.h"          // scheduler_get_current_process
#include "modules/process_management/core/signal/signal.h"                // ITIMER_REAL, ITIMERS
#include "modules/device_management/timer/timer.h"                        // TIMER_HZ
#include "arch/x86/cpu/cpu.h"                                             // cpu_irq_save, cpu_irq_restore
#include "modules/process_management/security/sleep_overflow/defence/sleep_bounds/def_sleep_bounds.h"  // def_sleep_time_ticks
#include "modules/process_management/security/timer_flood/defence/timer_limits/def_timer_limits.h"     // def_itimer_allowed

#define USEC_PER_TICK (1000000 / TIMER_HZ)

int sys_setitimer(int which, const setitimer_val_t *new_value, setitimer_val_t *old_value) {
    process_t *self = scheduler_get_current_process();
    if (!self || which < 0 || which >= ITIMERS)
        return -1;

    uint32_t value = 0, interval = 0;
    if (new_value) {
        // DEFENCE sleep_bounds: valid times, converted without overflow.
        if (!def_sleep_time_ticks(new_value->value_sec, new_value->value_usec, 1000000u, &value) ||
            !def_sleep_time_ticks(new_value->interval_sec, new_value->interval_usec, 1000000u, &interval))
            return -1;
        // DEFENCE timer_limits: periodic timers per user are counted.
        if (value && interval && !def_itimer_allowed(self, which))
            return -1;
    }

    uint32_t flags = cpu_irq_save();
    if (old_value) {
        old_value->value_sec     = (int32_t)(self->itimer_value[which] / TIMER_HZ);
        old_value->value_usec    = (int32_t)(self->itimer_value[which] % TIMER_HZ) * USEC_PER_TICK;
        old_value->interval_sec  = (int32_t)(self->itimer_interval[which] / TIMER_HZ);
        old_value->interval_usec = (int32_t)(self->itimer_interval[which] % TIMER_HZ) * USEC_PER_TICK;
    }
    if (new_value) {
        self->itimer_value[which]    = value;
        self->itimer_interval[which] = value ? interval : 0;
    }
    cpu_irq_restore(flags);
    return 0;
}

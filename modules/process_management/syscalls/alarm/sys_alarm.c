/*
 * sys_alarm.c - alarm(seconds): send SIGALRM to the caller later.
 *
 * A new alarm replaces the old one; 0 cancels it. Returns the seconds
 * that were left of the old alarm (rounded up), 0 if there was none.
 * alarm() is ITIMER_REAL (setitimer) without an interval: the countdown
 * is process_t.itimer_value[ITIMER_REAL]; signal_tick() (core/signal)
 * counts it down on every timer tick and sends SIGALRM at 0.
 *
 * Uses: scheduler_get_current_process (core/scheduler), TIMER_HZ
 *       (device_management), cpu_irq_save (arch/x86/cpu).
 */
#include "modules/process_management/syscalls/alarm/sys_alarm.h"  // this file
#include "modules/process_management/core/scheduler/scheduler.h"  // scheduler_get_current_process
#include "modules/process_management/core/signal/signal.h"        // ITIMER_REAL
#include "modules/device_management/timer/timer.h"                // TIMER_HZ
#include "arch/x86/cpu/cpu.h"                                     // cpu_irq_save, cpu_irq_restore

#define MAX_SECONDS (0xFFFFFFFFu / TIMER_HZ)    // more would overflow the tick count

uint32_t sys_alarm(uint32_t seconds) {
    process_t *self = scheduler_get_current_process();
    if (!self)
        return 0;
    if (seconds > MAX_SECONDS)
        seconds = MAX_SECONDS;

    uint32_t flags = cpu_irq_save();
    uint32_t left = (self->itimer_value[ITIMER_REAL] + TIMER_HZ - 1) / TIMER_HZ;
    self->itimer_value[ITIMER_REAL]    = seconds * TIMER_HZ;
    self->itimer_interval[ITIMER_REAL] = 0;
    cpu_irq_restore(flags);
    return left;
}

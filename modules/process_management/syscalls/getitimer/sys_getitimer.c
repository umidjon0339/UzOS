/*
 * sys_getitimer.c - getitimer(which, cur): read an interval timer.
 *
 * cur->value is the time left until it fires (0 = not armed), in whole
 * ticks of 10 ms; cur->interval is how often it fires again. Returns 0,
 * or -1 for a bad 'which'.
 *
 * Uses: scheduler_get_current_process (core/scheduler), ITIMERS
 *       (core/signal), TIMER_HZ (device_management).
 */
#include "modules/process_management/syscalls/getitimer/sys_getitimer.h"  // this file
#include "modules/process_management/core/scheduler/scheduler.h"          // scheduler_get_current_process
#include "modules/process_management/core/signal/signal.h"                // ITIMERS
#include "modules/device_management/timer/timer.h"                        // TIMER_HZ

#define USEC_PER_TICK (1000000 / TIMER_HZ)

int sys_getitimer(int which, getitimer_val_t *cur) {
    process_t *self = scheduler_get_current_process();
    if (!self || !cur || which < 0 || which >= ITIMERS)
        return -1;
    cur->value_sec     = (int32_t)(self->itimer_value[which] / TIMER_HZ);
    cur->value_usec    = (int32_t)(self->itimer_value[which] % TIMER_HZ) * USEC_PER_TICK;
    cur->interval_sec  = (int32_t)(self->itimer_interval[which] / TIMER_HZ);
    cur->interval_usec = (int32_t)(self->itimer_interval[which] % TIMER_HZ) * USEC_PER_TICK;
    return 0;
}

/*
 * sys_sleep.c - sleep(): stop running for 'ms' milliseconds.
 *
 * The process is marked SLEEPING with a tick count. The scheduler skips
 * it, and every timer tick counts it down; at 0 it becomes READY again.
 * Sleeping costs no CPU time, unlike a busy loop.
 *
 * Uses: the scheduler (current process, yield), TIMER_HZ (device_management
 *       timer): at 100 ticks per second, one tick = 10 ms.
 */

#include "sys_sleep.h"
#include "scheduler.h"
#include "timer.h"

#define MS_PER_TICK (1000 / TIMER_HZ)

void sys_sleep(uint32_t ms) {
    process_t *self = scheduler_get_current_process();
    if (!self)
        return;

    uint32_t ticks = (ms + MS_PER_TICK - 1) / MS_PER_TICK;     // round up
    self->sleep_ticks = ticks ? ticks : 1;
    self->state = PROCESS_STATE_SLEEPING;

    // Normally one yield is enough. Looping also covers the case where
    // there was nobody to switch to, so we never return early.
    while (self->state == PROCESS_STATE_SLEEPING)
        scheduler_yield();
}

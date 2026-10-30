/*
 * sys_times.c - times(buf): where the caller's CPU time went, in ticks.
 *
 * UzOS's timer ticks 100 times a second, the same as Linux's clock tick
 * (USER_HZ), so the numbers need no conversion. Returns the ticks since
 * boot: a program measures elapsed time by calling times() twice.
 *
 * Uses: scheduler_get_current_process (core/scheduler), timer_ticks
 *       (device_management).
 */
#include "modules/process_management/syscalls/times/sys_times.h"         // this file
#include "modules/process_management/core/scheduler/scheduler.h"         // scheduler_get_current_process
#include "modules/device_management/timer/timer.h"                       // timer_ticks

uint32_t sys_times(tms_t *buf) {
    process_t *self = scheduler_get_current_process();
    if (self && buf) {
        buf->utime  = self->user_ticks;
        buf->stime  = self->kernel_ticks;
        buf->cutime = self->child_user_ticks;
        buf->cstime = self->child_kernel_ticks;
    }
    return timer_ticks;
}

/*
 * sys_getrusage.c - getrusage(who, usage).
 *
 *   RUSAGE_SELF / RUSAGE_THREAD   what the caller used so far
 *   RUSAGE_CHILDREN               what its finished children used; the
 *                                 reaper adds each one to the parent
 *                                 (cpu_management core/task)
 * Fills CPU time in ring 3 / ring 0, maxrss (KiB: UzOS processes have
 * one 4 KiB kernel stack), and the context switches. Returns 0 or -1.
 *
 * Uses: scheduler_get_current_process (core/scheduler), TIMER_HZ
 *       (device_management), memset (lib/string).
 */
#include "modules/process_management/syscalls/getrusage/sys_getrusage.h"   // this file
#include "modules/process_management/core/scheduler/scheduler.h"           // scheduler_get_current_process
#include "modules/device_management/timer/timer.h"                         // TIMER_HZ
#include "lib/string/string.h"                                             // memset

static void ticks_to_timeval(uint32_t ticks, uint32_t *sec, uint32_t *usec) {
    *sec  = ticks / TIMER_HZ;
    *usec = (ticks % TIMER_HZ) * (1000000 / TIMER_HZ);
}

int sys_getrusage(int who, rusage_t *usage) {
    process_t *self = scheduler_get_current_process();
    if (!self || !usage || (who != RUSAGE_SELF && who != RUSAGE_CHILDREN && who != RUSAGE_THREAD))
        return -1;

    int children = who == RUSAGE_CHILDREN;
    memset(usage, 0, sizeof(*usage));
    ticks_to_timeval(children ? self->child_user_ticks : self->user_ticks,
                     &usage->utime_sec, &usage->utime_usec);
    ticks_to_timeval(children ? self->child_kernel_ticks : self->kernel_ticks,
                     &usage->stime_sec, &usage->stime_usec);
    usage->maxrss = PROCESS_STACK_SIZE / 1024;
    usage->nvcsw  = children ? self->child_nvcsw : self->nvcsw;
    usage->nivcsw = children ? self->child_nivcsw : self->nivcsw;
    return 0;
}

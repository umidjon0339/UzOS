/*
 * sys_pause.c - pause(): sleep until a signal that is not blocked comes.
 *
 * Ignored signals never wake it (they are thrown away when sent). It
 * always returns -1 (EINTR); the signal is delivered on the way out, so
 * its handler has run when the program sees -1. A deadly signal ends
 * the program instead.
 *
 * Uses: scheduler_get_current_process (core/scheduler), signal_deliverable
 *       (core/signal), sys_sleep_interruptible (syscalls/sleep).
 */
#include "modules/process_management/syscalls/pause/sys_pause.h"  // this file
#include "modules/process_management/core/scheduler/scheduler.h"  // scheduler_get_current_process
#include "modules/process_management/core/signal/signal.h"        // signal_deliverable
#include "modules/process_management/syscalls/sleep/sys_sleep.h"  // sys_sleep_interruptible

#define NAP_MS 1000

int sys_pause(void) {
    process_t *self = scheduler_get_current_process();
    if (!self)
        return -1;
    while (!signal_deliverable(self))
        sys_sleep_interruptible(NAP_MS);
    return -1;
}

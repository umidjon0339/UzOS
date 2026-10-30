/*
 * sys_rt_sigsuspend.c - rt_sigsuspend(mask, sigsetsize).
 *
 * "Unblock these and wait" in ONE step: the blocked mask becomes 'mask'
 * until a signal arrives, so a signal can never slip in between an
 * unblock and a pause(). The old mask comes back after the handler
 * (delivery saves it in the frame; rt_sigreturn restores it). Always
 * returns -1 (EINTR), like Linux. For ring 3 programs.
 *
 * Uses: scheduler_get_current_process (core/scheduler), signal_deliverable
 *       (core/signal), sys_sleep_interruptible (syscalls/sleep).
 */
#include "modules/process_management/syscalls/rt_sigsuspend/sys_rt_sigsuspend.h"  // this file
#include "modules/process_management/core/scheduler/scheduler.h"                  // scheduler_get_current_process
#include "modules/process_management/core/signal/signal.h"                        // signal_deliverable, SIG_UNBLOCKABLE
#include "modules/process_management/syscalls/sleep/sys_sleep.h"                  // sys_sleep_interruptible

#define NAP_MS 1000             // woken at once by a signal; the nap is a fallback

int sys_rt_sigsuspend(const uint32_t *mask, uint32_t sigsetsize) {
    process_t *self = scheduler_get_current_process();
    if (!self || !mask || (sigsetsize != 4 && sigsetsize != 8))
        return -1;

    self->sig_saved_mask = self->sig_blocked;
    self->sig_restore_mask = 1;
    self->sig_blocked = mask[0] & ~SIG_UNBLOCKABLE;

    while (!signal_deliverable(self))
        sys_sleep_interruptible(NAP_MS);
    return -1;                  // EINTR: delivery runs on the way out
}

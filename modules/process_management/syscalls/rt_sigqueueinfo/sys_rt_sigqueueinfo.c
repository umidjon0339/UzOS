/*
 * sys_rt_sigqueueinfo.c - rt_sigqueueinfo(pid, sig, info): sigqueue(3).
 *
 * Like kill(), but the signal carries info->value, and every one
 * arrives: two queued SIGUSR1s run the handler twice, with each value.
 * Only the value is taken from 'info': the kernel writes who sent it
 * (pid, uid) and why (SI_QUEUE) itself, so a sender cannot pretend to
 * be the kernel or another process.
 *
 * Same permission rule as kill() (signal_permission); the queue has a
 * share per user (sigqueue_limit, inside sigqueue_add).
 *
 * Uses: scheduler_get_current_process, process_get_by_pid (core/process),
 *       sigqueue_add / sigqueue_take / signal_send_info (core/signal),
 *       def_signal_allowed (security/signal_abuse/defence/signal_permission).
 */
#include "modules/process_management/syscalls/rt_sigqueueinfo/sys_rt_sigqueueinfo.h"                           // this file
#include "modules/process_management/core/scheduler/scheduler.h"                                               // scheduler_get_current_process
#include "modules/process_management/security/signal_abuse/defence/signal_permission/def_signal_permission.h"  // def_signal_allowed

int sys_rt_sigqueueinfo(uint32_t pid, int sig, const k_siginfo_t *info) {
    process_t *self = scheduler_get_current_process();
    process_t *t = process_get_by_pid(pid);
    if (!self || !t || !info || sig < 1 || sig >= SIGNAL_COUNT || sig == SIGKILL || sig == SIGSTOP)
        return -1;
    if (!def_signal_allowed(self->uid, t->uid, t->is_system))
        return -1;

    k_siginfo_t queued = { sig, 0, SI_QUEUE, self->pid, self->uid, info->value };
    if (!sigqueue_add(t, &queued))
        return -1;                          // EAGAIN: this user's share is used up
    signal_send_info(t, sig, SI_QUEUE, info->value);
    if (!signal_pending(t, sig))            // ignored: it was thrown away
        sigqueue_take(t, sig, 0);
    return 0;
}

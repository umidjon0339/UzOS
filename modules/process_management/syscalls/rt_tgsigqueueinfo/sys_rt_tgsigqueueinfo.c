/*
 * sys_rt_tgsigqueueinfo.c - rt_tgsigqueueinfo(tgid, tid, sig, info).
 *
 * rt_sigqueueinfo() for one THREAD: 'tid' must belong to thread group
 * 'tgid' (the same safety check tgkill() makes, so a reused tid can
 * never hit the wrong program). Value, sender and permission as in
 * rt_sigqueueinfo().
 *
 * Uses: scheduler_get_current_process, process_get_by_pid (core/process),
 *       sigqueue_add / sigqueue_take / signal_send_info (core/signal),
 *       def_signal_allowed (security/signal_abuse/defence/signal_permission).
 */
#include "modules/process_management/syscalls/rt_tgsigqueueinfo/sys_rt_tgsigqueueinfo.h"                       // this file
#include "modules/process_management/core/scheduler/scheduler.h"                                               // scheduler_get_current_process
#include "modules/process_management/security/signal_abuse/defence/signal_permission/def_signal_permission.h"  // def_signal_allowed

int sys_rt_tgsigqueueinfo(uint32_t tgid, uint32_t tid, int sig, const k_siginfo_t *info) {
    process_t *self = scheduler_get_current_process();
    process_t *t = process_get_by_pid(tid);
    if (!self || !t || t->tgid != tgid || !info ||
        sig < 1 || sig >= SIGNAL_COUNT || sig == SIGKILL || sig == SIGSTOP)
        return -1;
    if (!def_signal_allowed(self->uid, t->uid, t->is_system))
        return -1;

    k_siginfo_t queued = { sig, 0, SI_QUEUE, self->pid, self->uid, info->value };
    if (!sigqueue_add(t, &queued))
        return -1;
    signal_send_info(t, sig, SI_QUEUE, info->value);
    if (!signal_pending(t, sig))
        sigqueue_take(t, sig, 0);
    return 0;
}

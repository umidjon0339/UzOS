/*
 * sys_tgkill.c - tgkill(tgid, tid, sig): signal a thread, but only if it
 * really belongs to thread group 'tgid'. This closes a tkill race: a tid
 * could be reused by another group between look-up and signal. Checking
 * the tgid makes sure you signal the thread you meant.
 *
 * Uses: scheduler_get_current_process, process_get_by_pid, process_kill
 *       (core/process); signal_send, SIGKILL (core/signal);
 *       def_signal_allowed (signal_permission); process_t.tgid.
 */
#include "modules/process_management/syscalls/tgkill/sys_tgkill.h"                                             // this file
#include "modules/process_management/core/process/process.h"                                                   // process_t, process_get_by_pid, process_kill
#include "modules/process_management/core/scheduler/scheduler.h"                                               // scheduler_get_current_process
#include "modules/process_management/core/signal/signal.h"                                                     // SIGKILL, signal_send
#include "modules/process_management/security/signal_abuse/defence/signal_permission/def_signal_permission.h"  // def_signal_allowed

int sys_tgkill(uint32_t tgid, uint32_t tid, int sig) {
    process_t *self = scheduler_get_current_process();
    process_t *t = process_get_by_pid(tid);
    if (!t || t->tgid != tgid)           // the tid must be in this group
        return -1;
    if (!def_signal_allowed(self ? self->uid : 0, t->uid, t->is_system))
        return -1;
    if (sig == SIGKILL) { process_kill(tid); return 0; }
    signal_send(t, sig);
    return 0;
}

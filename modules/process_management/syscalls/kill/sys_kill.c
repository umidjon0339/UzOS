/*
 * sys_kill.c - kill(pid, sig): send a signal to a process.
 *
 * Checks permission (signal_permission: your own process, or CAP_KILL,
 * and never a system process). SIGKILL ends the process at once; any
 * other signal is delivered as a pending bit the process may act on.
 *
 * Uses: scheduler_get_current_process, process_get_by_pid, process_kill
 *       (core/process); signal_send, SIGKILL (core/signal);
 *       def_signal_allowed (security/signal_abuse/defence/signal_permission).
 */
#include "sys_kill.h"
#include "process.h"
#include "scheduler.h"
#include "signal.h"
#include "def_signal_permission.h"

int sys_kill(uint32_t pid, int sig) {
    process_t *self = scheduler_get_current_process();
    process_t *t = process_get_by_pid(pid);
    if (!t) return -1;
    if (!def_signal_allowed(self ? self->uid : 0, t->uid, t->is_system))
        return -1;
    if (sig == SIGKILL) { process_kill(pid); return 0; }   // forced
    signal_send(t, sig);                                    // delivered as pending
    return 0;
}

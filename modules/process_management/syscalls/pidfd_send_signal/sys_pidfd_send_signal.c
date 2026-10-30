/*
 * sys_pidfd_send_signal.c - pidfd_send_signal(pidfd, sig): send a signal
 * to the process a pidfd (pidfd_open) names.
 *
 * This is Linux's fix for the kill() race: between finding a pid and
 * killing it, the process may end and the number may come back as
 * someone else. The pidfd names the process itself, so once it ended
 * this fails (-1, ESRCH) instead of hitting the next one. Same
 * permission rule as kill (signal_permission).
 *
 * Uses: scheduler_get_current_process (core/scheduler), process_kill
 *       (core/process), pidfd_target (core/pidfd: pcb_generation inside),
 *       signal_send, SIGKILL (core/signal), def_signal_allowed
 *       (security/signal_abuse/defence/signal_permission).
 */
#include "modules/process_management/syscalls/pidfd_send_signal/sys_pidfd_send_signal.h"  // this file
#include "modules/process_management/core/scheduler/scheduler.h"                           // scheduler_get_current_process
#include "modules/process_management/core/pidfd/pidfd.h"                                   // pidfd_target
#include "modules/process_management/core/signal/signal.h"                                 // signal_send, SIGKILL
#include "modules/process_management/security/signal_abuse/defence/signal_permission/def_signal_permission.h"  // def_signal_allowed

int sys_pidfd_send_signal(int pidfd, int sig) {
    process_t *self = scheduler_get_current_process();
    process_t *t = pidfd_target(self, pidfd);
    if (!t || t->state == PROCESS_STATE_ZOMBIE || sig < 0 || sig >= SIGNAL_COUNT)
        return -1;                                  // ESRCH: it ended
    if (!def_signal_allowed(self->uid, t->uid, t->is_system))
        return -1;
    if (sig == 0)
        return 0;                                   // only "is it alive and may I?"
    if (sig == SIGKILL)
        return process_kill(t->pid) ? 0 : -1;
    signal_send(t, sig);
    return 0;
}

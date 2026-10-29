/*
 * sys_pidfd_send_signal.c - pidfd_send_signal(pidfd, sig): send a signal
 * using a file descriptor that refers to a process.
 *
 * A pidfd is Linux's fix for the kill() race where a pid is reused after
 * the target dies: the fd always names the same process, so you never
 * signal the wrong one. UzOS has no pidfd type yet; here the pidfd is
 * taken as the pid directly (PIDs are never reused, so the race the real
 * pidfd solves cannot happen in UzOS anyway). Same permission rule as kill.
 *
 * Uses: scheduler_get_current_process, process_get_by_pid, process_kill
 *       (core/process); signal_send, SIGKILL (core/signal);
 *       def_signal_allowed (signal_permission).
 */
#include "sys_pidfd_send_signal.h"
#include "process.h"
#include "scheduler.h"
#include "signal.h"
#include "def_signal_permission.h"

int sys_pidfd_send_signal(int pidfd, int sig) {
    process_t *self = scheduler_get_current_process();
    process_t *t = process_get_by_pid((uint32_t)pidfd);   // pidfd == pid in UzOS
    if (!t) return -1;
    if (!def_signal_allowed(self ? self->uid : 0, t->uid, t->is_system))
        return -1;
    if (sig == SIGKILL) { process_kill((uint32_t)pidfd); return 0; }
    signal_send(t, sig);
    return 0;
}

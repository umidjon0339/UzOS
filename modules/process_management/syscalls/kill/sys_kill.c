/*
 * sys_kill.c - kill(pid, sig): send a signal to one process or a group.
 *
 *   pid > 0    that process
 *   pid == 0   every process in my process group
 *   pid == -1  every process I may signal (not system ones, not myself)
 *   pid < -1   every process in group -pid
 *
 * Each target is checked by signal_permission (your own process, or
 * CAP_KILL, never a system process). SIGKILL ends a process at once
 * (process_kill); sig 0 sends nothing, it only asks "may I?". Returns 0
 * if at least one process got it, -1 otherwise.
 *
 * Uses: scheduler_get_current_process, process_get_by_pid / process_get_table /
 *       process_kill (core/process); signal_send, SIGKILL (core/signal);
 *       caps_has (security_management); def_signal_allowed
 *       (security/signal_abuse/defence/signal_permission).
 */
#include "modules/process_management/syscalls/kill/sys_kill.h"         // this file
#include "modules/process_management/core/process/process.h"         // process_get_by_pid, process_kill
#include "modules/process_management/core/scheduler/scheduler.h"     // scheduler_get_current_process
#include "modules/process_management/core/signal/signal.h"           // SIGKILL, signal_send
#include "modules/security_management/core/caps/caps.h"              // caps_has, CAP_KILL
#include "modules/process_management/security/signal_abuse/defence/signal_permission/def_signal_permission.h"  // def_signal_allowed

static int send_one(process_t *self, process_t *t, int sig) {
    if (!def_signal_allowed(self ? self->uid : 0, t->uid, t->is_system))
        return 0;
    if (sig == SIGKILL)
        return process_kill(t->pid);            // forced
    if (sig)
        signal_send(t, sig);                    // delivered as pending
    return 1;
}

int sys_kill(int32_t pid, int sig) {
    process_t *self = scheduler_get_current_process();
    if (sig < 0 || sig >= SIGNAL_COUNT)
        return -1;
    if (pid > 0) {
        process_t *t = process_get_by_pid((uint32_t)pid);
        return t && send_one(self, t, sig) ? 0 : -1;
    }

    uint32_t uid = self ? self->uid : 0;
    uint32_t group = pid == 0 ? (self ? self->pgid : 0) : (uint32_t)-pid;
    process_t *table = process_get_table();
    int sent = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_t *t = &table[i];
        if (t->state == PROCESS_STATE_FREE || t->state == PROCESS_STATE_ZOMBIE || t->is_system)
            continue;
        if (pid == -1) {                        // all I may: skip, do not ask
            if (t == self || (t->uid != uid && !caps_has(uid, CAP_KILL)))
                continue;
        } else if (t->pgid != group) {
            continue;
        }
        sent += send_one(self, t, sig);
    }
    return sent ? 0 : -1;
}

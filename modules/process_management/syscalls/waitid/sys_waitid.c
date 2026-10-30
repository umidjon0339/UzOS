/*
 * sys_waitid.c - waitid(idtype, id, infop, options): the richer wait.
 *
 *   P_ALL     any child            P_PID    the child 'id'
 *   P_PGID    any child in group 'id'      P_PIDFD  the child pidfd 'id' names
 *
 * Fills 'infop' instead of a status word: who ended (pid, uid) and how
 * (code CLD_EXITED + exit code, or CLD_KILLED / CLD_DUMPED + the
 * signal). Returns the child's pid (Linux returns 0; UzOS gives the pid,
 * which infop->pid also holds), 0 with WNOHANG if none ended yet, -1 on
 * error. The record is taken from core/exitlog, like every wait.
 *
 * Uses: scheduler_get_current_process, process_get_table / process_get_by_pid
 *       (core/process), exitlog_take / exitlog_find (core/exitlog),
 *       pidfd_target (core/pidfd), sys_sleep (syscalls/sleep),
 *       def_wait_allowed (security/wait_steal/defence/child_only), SIGCHLD.
 */
#include "modules/process_management/syscalls/waitid/sys_waitid.h"     // this file
#include "modules/process_management/core/scheduler/scheduler.h"       // scheduler_get_current_process
#include "modules/process_management/core/exitlog/exitlog.h"           // exitlog_take, exitlog_find
#include "modules/process_management/core/pidfd/pidfd.h"               // pidfd_target
#include "modules/process_management/core/signal/signal.h"             // SIGCHLD
#include "modules/process_management/syscalls/sleep/sys_sleep.h"       // sys_sleep
#include "modules/process_management/security/wait_steal/defence/child_only/def_child_only.h"  // def_wait_allowed

#define POLL_MS     10
#define CLD_EXITED  1
#define CLD_KILLED  2
#define CLD_DUMPED  3

// Any child of 'self' (not a thread) that 'sel' (as wait4's pid) selects?
static int has_child(const process_t *self, int32_t sel) {
    process_t *table = process_get_table();
    for (int i = 0; i < MAX_PROCESSES; i++) {
        const process_t *c = &table[i];
        if (c->state == PROCESS_STATE_FREE || c->ppid != self->pid || (c->tgid && c->tgid != c->pid))
            continue;
        if (sel == -1 || (sel > 0 && c->pid == (uint32_t)sel) || (sel < -1 && c->pgid == (uint32_t)-sel))
            return 1;
    }
    return 0;
}

int sys_waitid(int idtype, uint32_t id, waitid_info_t *infop, uint32_t options) {
    process_t *self = scheduler_get_current_process();
    if (!self || (options & ~(WEXITED | WNOHANG_ID)))
        return -1;

    int32_t sel;                                // as wait4's pid argument
    if (idtype == P_ALL) {
        sel = -1;
    } else if (idtype == P_PGID) {
        sel = id > 1 ? -(int32_t)id : -1;
    } else if (idtype == P_PID || idtype == P_PIDFD) {
        uint32_t pid = id;
        if (idtype == P_PIDFD) {
            process_t *t = pidfd_target(self, (int)id);
            if (!t)
                return -1;                      // not a pidfd, or already reaped
            pid = t->pid;
        }
        process_t *c = process_get_by_pid(pid);
        exit_record_t r;
        uint32_t parent = c ? c->ppid : exitlog_find(pid, &r) ? r.ppid : 0;
        if (!parent || !def_wait_allowed(self, pid, parent))   // DEFENCE child_only
            return -1;
        sel = (int32_t)pid;
    } else {
        return -1;
    }

    for (;;) {
        exit_record_t r;
        if (exitlog_take(self->pid, sel, self->pgid, &r)) {
            if (infop) {
                int sig = r.status & 0x7F;
                infop->signo  = SIGCHLD;
                infop->error  = 0;
                infop->code   = !sig ? CLD_EXITED : (r.status & 0x80) ? CLD_DUMPED : CLD_KILLED;
                infop->pid    = r.pid;
                infop->uid    = r.uid;
                infop->status = sig ? sig : (r.status >> 8) & 0xFF;
            }
            return (int)r.pid;
        }
        if (!has_child(self, sel))
            return -1;
        if (options & WNOHANG_ID)
            return 0;
        sys_sleep(POLL_MS);
    }
}

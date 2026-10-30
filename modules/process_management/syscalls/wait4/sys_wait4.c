/*
 * sys_wait4.c - wait4(pid, wstatus, options, rusage): the full wait.
 *
 * Which child (as in Linux):
 *   pid > 0     that child          pid == -1   any child
 *   pid == 0    any child in my process group
 *   pid < -1    any child in process group -pid
 *
 * When one has ended, its record is TAKEN from core/exitlog (the reaper
 * wrote it), so each child is reported once:
 *   *wstatus = how it ended:  exit(n) -> n << 8,  signal s -> s (| 0x80 core)
 *   *rusage  = the CPU time and context switches it (and its children) used
 *   returns its pid
 * -1 = no such child (ECHILD), not your child (child_only), or a signal
 * came while waiting (EINTR). With WNOHANG, 0 = children run, none ended.
 * Threads (clone CLONE_THREAD) are not waited for, as in Linux.
 *
 * Uses: scheduler_get_current_process, process_get_table / process_get_by_pid
 *       (core/process), exitlog_take / exitlog_find (core/exitlog),
 *       signal_deliverable (core/signal), sys_sleep_interruptible
 *       (syscalls/sleep), TIMER_HZ (device_management), def_wait_allowed
 *       (security/wait_steal/defence/child_only).
 */
#include "modules/process_management/syscalls/wait4/sys_wait4.h"                 // this file
#include "modules/process_management/core/scheduler/scheduler.h"                 // scheduler_get_current_process
#include "modules/process_management/core/exitlog/exitlog.h"                     // exitlog_take, exitlog_find
#include "modules/process_management/core/signal/signal.h"                       // signal_deliverable
#include "modules/process_management/syscalls/sleep/sys_sleep.h"                 // sys_sleep_interruptible
#include "modules/process_management/security/wait_steal/defence/child_only/def_child_only.h"  // def_wait_allowed
#include "modules/device_management/timer/timer.h"                               // TIMER_HZ
#include "lib/string/string.h"                                                   // memset

#define POLL_MS 10

static int selected(const process_t *c, int32_t pid, uint32_t my_pgid) {
    if (pid > 0)   return c->pid == (uint32_t)pid;
    if (pid == -1) return 1;
    if (pid == 0)  return c->pgid == my_pgid;
    return c->pgid == (uint32_t)-pid;
}

// Does 'self' still have a child (alive or not yet reaped) that 'pid' selects?
static int has_child(const process_t *self, int32_t pid) {
    process_t *table = process_get_table();
    for (int i = 0; i < MAX_PROCESSES; i++) {
        const process_t *c = &table[i];
        if (c->state != PROCESS_STATE_FREE && c->ppid == self->pid &&
            (!c->tgid || c->tgid == c->pid) && selected(c, pid, self->pgid))
            return 1;
    }
    return 0;
}

static void ticks_to_timeval(uint32_t ticks, uint32_t *sec, uint32_t *usec) {
    *sec  = ticks / TIMER_HZ;
    *usec = (ticks % TIMER_HZ) * (1000000 / TIMER_HZ);
}

int sys_wait4(int32_t pid, int32_t *wstatus, uint32_t options, wait4_rusage_t *rusage) {
    process_t *self = scheduler_get_current_process();
    if (!self || (options & ~WNOHANG))
        return -1;

    // DEFENCE child_only: a named pid must be our own child.
    if (pid > 0) {
        process_t *c = process_get_by_pid((uint32_t)pid);
        exit_record_t r;
        uint32_t parent = c ? c->ppid : exitlog_find((uint32_t)pid, &r) ? r.ppid : 0;
        if (!c && !parent)
            return -1;                              // ECHILD: no such process
        if (!def_wait_allowed(self, (uint32_t)pid, parent))
            return -1;
    }

    for (;;) {
        exit_record_t r;
        if (exitlog_take(self->pid, pid, self->pgid, &r)) {
            if (wstatus)
                *wstatus = r.status;
            if (rusage) {
                memset(rusage, 0, sizeof(*rusage));
                ticks_to_timeval(r.user_ticks, &rusage->utime_sec, &rusage->utime_usec);
                ticks_to_timeval(r.kernel_ticks, &rusage->stime_sec, &rusage->stime_usec);
                rusage->maxrss = PROCESS_STACK_SIZE / 1024;
                rusage->nvcsw  = r.nvcsw;
                rusage->nivcsw = r.nivcsw;
            }
            return (int)r.pid;
        }
        if (!has_child(self, pid))
            return -1;                              // ECHILD: nothing left to wait for
        if (options & WNOHANG)
            return 0;
        if (!sys_sleep_interruptible(POLL_MS) && signal_deliverable(self))
            return -1;                              // EINTR: a signal came
    }
}

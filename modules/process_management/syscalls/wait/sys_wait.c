/*
 * sys_wait.c - wait(): block until one of the caller's children ends.
 *
 * Returns the pid of a finished child, or -1 if the caller has none.
 * The child's record is taken from core/exitlog (the reaper wrote it),
 * so the same child is never reported twice; wait4() also gives the
 * exit status. Threads are not waited for.
 *
 * Uses: scheduler_get_current_process, process_get_table (core/process),
 *       exitlog_take (core/exitlog), sys_sleep (syscalls/sleep).
 */
#include "modules/process_management/syscalls/wait/sys_wait.h"      // this file
#include "modules/process_management/core/scheduler/scheduler.h"    // scheduler_get_current_process
#include "modules/process_management/core/exitlog/exitlog.h"        // exitlog_take
#include "modules/process_management/syscalls/sleep/sys_sleep.h"    // sys_sleep

#define POLL_MS 10

static int has_child(const process_t *self) {
    process_t *table = process_get_table();
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (table[i].state != PROCESS_STATE_FREE && table[i].ppid == self->pid &&
            (!table[i].tgid || table[i].tgid == table[i].pid))
            return 1;
    return 0;
}

int sys_wait(void) {
    process_t *self = scheduler_get_current_process();
    if (!self) return -1;

    for (;;) {
        exit_record_t r;
        if (exitlog_take(self->pid, -1, 0, &r))
            return (int)r.pid;
        if (!has_child(self))
            return -1;                      // nothing left to wait for
        sys_sleep(POLL_MS);
    }
}

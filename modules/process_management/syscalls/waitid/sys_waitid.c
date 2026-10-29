/*
 * sys_waitid.c - waitid(idtype, id): the richer wait interface.
 *
 * idtype P_ALL waits for any child; P_PID waits for the child 'id'.
 * Linux also fills a siginfo_t and takes option flags; UzOS keeps no
 * exit code, so this returns the finished child's pid (or -1) and
 * ignores the extra arguments. Self-contained: it does its own polling.
 *
 * Uses: scheduler_get_current_process, process_get_by_pid,
 *       process_get_table, MAX_PROCESSES, PROCESS_STATE_* (core/process);
 *       sys_sleep (syscalls/sleep).
 */
#include "sys_waitid.h"
#include "process.h"
#include "scheduler.h"
#include "sys_sleep.h"

#define POLL_MS 10

int sys_waitid(int idtype, uint32_t id) {
    process_t *self = scheduler_get_current_process();
    if (!self) return -1;

    if (idtype == P_PID) {
        process_t *c = process_get_by_pid(id);
        if (!c || c->ppid != self->pid) return -1;
        while (c && c->state != PROCESS_STATE_ZOMBIE) {
            sys_sleep(POLL_MS);
            c = process_get_by_pid(id);
        }
        return (int)id;
    }

    // P_ALL (and anything else): any child
    process_t *table = process_get_table();
    for (;;) {
        int have_child = 0;
        for (int i = 0; i < MAX_PROCESSES; i++) {
            process_t *c = &table[i];
            if (c->state == PROCESS_STATE_FREE || c->ppid != self->pid)
                continue;
            have_child = 1;
            if (c->state == PROCESS_STATE_ZOMBIE)
                return (int)c->pid;
        }
        if (!have_child) return -1;
        sys_sleep(POLL_MS);
    }
}

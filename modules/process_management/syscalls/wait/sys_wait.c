/*
 * sys_wait.c - wait(): block until one of the caller's children finishes.
 *
 * Returns the pid of a finished child, or -1 if the caller has none.
 * NOTE: UzOS has a GLOBAL reaper that frees a zombie on its own, so a
 * child may be freed before wait() sees it as a zombie; wait() is
 * therefore best-effort - it is how a parent blocks for a child, not a
 * place that collects an exit code (UzOS keeps none).
 *
 * Uses: scheduler_get_current_process, process_get_table, MAX_PROCESSES,
 *       PROCESS_STATE_* (core/process); sys_sleep (syscalls/sleep).
 */
#include "sys_wait.h"
#include "process.h"
#include "scheduler.h"
#include "sys_sleep.h"

#define POLL_MS 10

int sys_wait(void) {
    process_t *self = scheduler_get_current_process();
    if (!self) return -1;
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
        if (!have_child) return -1;      // nothing left to wait for
        sys_sleep(POLL_MS);
    }
}

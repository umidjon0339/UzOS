/*
 * sys_waitpid.c - waitpid(pid): block until a specific child finishes.
 *
 * Returns 'pid' once that child is a zombie or already gone, or -1 if it
 * is not the caller's child. (See sys_wait on UzOS's global reaper: no
 * exit code is kept; this is how you block for one child.)
 *
 * Uses: scheduler_get_current_process, process_get_by_pid,
 *       PROCESS_STATE_ZOMBIE (core/process); sys_sleep (syscalls/sleep).
 */
#include "sys_waitpid.h"
#include "process.h"
#include "scheduler.h"
#include "sys_sleep.h"

#define POLL_MS 10

int sys_waitpid(uint32_t pid) {
    process_t *self = scheduler_get_current_process();
    if (!self) return -1;

    process_t *c = process_get_by_pid(pid);
    if (!c || c->ppid != self->pid)         // not our (live) child
        return -1;

    while (c && c->state != PROCESS_STATE_ZOMBIE) {
        sys_sleep(POLL_MS);
        c = process_get_by_pid(pid);        // becomes NULL when reaped
    }
    return (int)pid;                        // finished
}

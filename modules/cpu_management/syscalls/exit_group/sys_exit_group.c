/*
 * sys_exit_group.c - exit_group(): the end of a whole program.
 *
 *   exit()         ends only the calling THREAD; the others go on
 *   exit_group()   ends every thread of the group (same getpid())
 *
 * The C library's exit() really calls exit_group(): when main()
 * returns, all threads must stop, or the program would keep running
 * without its main thread.
 *
 * How: kill every other process with our thread group id (tgid), then
 * end ourselves. process_kill() checks kill_guard: they are our own
 * threads, so it allows it.
 *
 * Uses: process_kill / process_exit / the process table and the current
 *       process (process_management).
 */

#include "modules/cpu_management/syscalls/exit_group/sys_exit_group.h"
#include "modules/process_management/core/scheduler/scheduler.h"    // the current process

void sys_exit_group(int status) {
    process_t *self = scheduler_get_current_process();
    process_t *table = process_get_table();
    (void)status;                       // UzOS has no wait() to collect it

    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_t *p = &table[i];
        if (p != self && p->tgid == self->tgid && p->state != PROCESS_STATE_FREE &&
            p->state != PROCESS_STATE_ZOMBIE)
            process_kill(p->pid);
    }
    process_exit();
}

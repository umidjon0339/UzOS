/*
 * sys_getpid.c - getpid(): return the process ID of the caller.
 *
 * A "process" in Linux terms is a thread GROUP: the first thread and
 * every thread it made with clone(CLONE_THREAD). They all answer the
 * same getpid(), the group's id (tgid). A process without threads is a
 * group of one, so its tgid is simply its pid.
 *
 *   thread          pid (gettid)    tgid (getpid)
 *   main            7               7
 *   clone()d        8               7
 *
 * Uses: scheduler_get_current_process (process_management/core/scheduler).
 */

#include "modules/cpu_management/syscalls/getpid/sys_getpid.h"     // this syscall
#include "modules/process_management/core/scheduler/scheduler.h"    // scheduler_get_current_process

uint32_t sys_getpid(void) {
    process_t *self = scheduler_get_current_process();
    if (!self)
        return 0;
    return self->tgid ? self->tgid : self->pid;
}

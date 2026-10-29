/*
 * sys_setpgid.c - setpgid(pid, pgid): set a process's group.
 *
 * pid 0 means the caller; pgid 0 means "a new group named by the pid".
 * You may only move your own process (or one you own); root may move any.
 * Uses: scheduler_get_current_process, process_get_by_pid (core/process).
 */
#include "sys_setpgid.h"
#include "process.h"
#include "scheduler.h"

int sys_setpgid(uint32_t pid, uint32_t pgid) {
    process_t *self = scheduler_get_current_process();
    if (!self) return -1;
    process_t *t = pid ? process_get_by_pid(pid) : self;
    if (!t) return -1;
    if (self->uid != 0 && t->uid != self->uid)   // not yours, not root
        return -1;
    t->pgid = pgid ? pgid : t->pid;
    return 0;
}

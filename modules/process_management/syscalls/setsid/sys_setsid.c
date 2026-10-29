/*
 * sys_setsid.c - setsid(): the caller starts a new session and a new
 * process group, and becomes the leader of both (sid = pgid = its pid).
 * Returns the new session id.
 * Uses: scheduler_get_current_process (core/scheduler).
 */
#include "sys_setsid.h"
#include "process.h"
#include "scheduler.h"

int sys_setsid(void) {
    process_t *self = scheduler_get_current_process();
    if (!self) return -1;
    self->sid  = self->pid;     // new session, leader = self
    self->pgid = self->pid;     // new group too
    return (int)self->sid;
}

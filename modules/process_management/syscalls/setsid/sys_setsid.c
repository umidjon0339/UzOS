/*
 * sys_setsid.c - setsid(): the caller starts a new session and a new
 * process group, and becomes the leader of both (sid = pgid = its pid).
 * Returns the new session id, or -1 if the caller already leads a
 * process group (session_check: its group would be split in two).
 *
 * Uses: scheduler_get_current_process (core/scheduler), def_setsid_ok
 *       (security/pgid_hijack/defence/session_check).
 */
#include "modules/process_management/syscalls/setsid/sys_setsid.h"     // this file
#include "modules/process_management/core/process/process.h"         // process_t
#include "modules/process_management/core/scheduler/scheduler.h"     // scheduler_get_current_process
#include "modules/process_management/security/pgid_hijack/defence/session_check/def_session_check.h"  // def_setsid_ok

int sys_setsid(void) {
    process_t *self = scheduler_get_current_process();
    if (!self) return -1;
    if (!def_setsid_ok(self))   // DEFENCE session_check
        return -1;
    self->sid  = self->pid;     // new session, leader = self
    self->pgid = self->pid;     // new group too
    return (int)self->sid;
}

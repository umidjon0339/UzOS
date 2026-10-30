/*
 * sys_setpgid.c - setpgid(pid, pgid): put a process in a process group.
 *
 * pid 0 means the caller; pgid 0 means "a new group named by the pid".
 * session_check applies the POSIX rules: yourself or your child, within
 * your session, never a session leader, and only into a new group or
 * one that exists in that session.
 *
 * Uses: scheduler_get_current_process, process_get_by_pid (core/process),
 *       def_setpgid_ok (security/pgid_hijack/defence/session_check).
 */
#include "modules/process_management/syscalls/setpgid/sys_setpgid.h"     // this file
#include "modules/process_management/core/process/process.h"           // process_t, process_get_by_pid
#include "modules/process_management/core/scheduler/scheduler.h"       // scheduler_get_current_process
#include "modules/process_management/security/pgid_hijack/defence/session_check/def_session_check.h"  // def_setpgid_ok

int sys_setpgid(uint32_t pid, uint32_t pgid) {
    process_t *self = scheduler_get_current_process();
    if (!self) return -1;
    process_t *t = pid ? process_get_by_pid(pid) : self;
    if (!t) return -1;
    if (!pgid)
        pgid = t->pid;
    // DEFENCE session_check: groups stay inside their session.
    if (!def_setpgid_ok(self, t, pgid))
        return -1;
    t->pgid = pgid;
    return 0;
}

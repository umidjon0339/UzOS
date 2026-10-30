/*
 * sys_waitpid.c - waitpid(pid): block until a specific child ends.
 *
 * Returns 'pid' once that child has ended (its record is taken from
 * core/exitlog), or -1 if it is not the caller's child (child_only).
 * wait4() is the same with the exit status and resource usage.
 *
 * Uses: scheduler_get_current_process, process_get_by_pid (core/process),
 *       exitlog_take / exitlog_find (core/exitlog), sys_sleep
 *       (syscalls/sleep), def_wait_allowed (security/wait_steal/defence/child_only).
 */
#include "modules/process_management/syscalls/waitpid/sys_waitpid.h"   // this file
#include "modules/process_management/core/scheduler/scheduler.h"       // scheduler_get_current_process
#include "modules/process_management/core/exitlog/exitlog.h"           // exitlog_take, exitlog_find
#include "modules/process_management/syscalls/sleep/sys_sleep.h"       // sys_sleep
#include "modules/process_management/security/wait_steal/defence/child_only/def_child_only.h"  // def_wait_allowed

#define POLL_MS 10

int sys_waitpid(uint32_t pid) {
    process_t *self = scheduler_get_current_process();
    if (!self || !pid) return -1;

    process_t *c = process_get_by_pid(pid);
    exit_record_t r;
    uint32_t parent = c ? c->ppid : exitlog_find(pid, &r) ? r.ppid : 0;
    if (!parent || !def_wait_allowed(self, pid, parent))   // DEFENCE child_only
        return -1;

    while (!exitlog_take(self->pid, (int32_t)pid, 0, 0)) {
        if (!process_get_by_pid(pid) && !exitlog_find(pid, 0))
            return -1;                      // gone without a record (log overflow)
        sys_sleep(POLL_MS);
    }
    return (int)pid;
}

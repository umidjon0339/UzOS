/*
 * sys_pidfd_open.c - pidfd_open(pid, flags): get a handle on a process.
 *
 * The handle (a file descriptor) names exactly this process for ever:
 * pidfd_send_signal, pidfd_getfd and waitid(P_PIDFD) use it, and they
 * fail once the process has ended, instead of reaching whoever comes
 * next. close() it like any file. Returns the descriptor, or -1.
 *
 * Uses: scheduler_get_current_process / process_get_by_pid (core/process),
 *       pidfd_create (core/pidfd), fd_alloc (io_management core/fd),
 *       def_pidfd_open_ok (security/pid_reuse_race/defence/pidfd_handle).
 */
#include "modules/process_management/syscalls/pidfd_open/sys_pidfd_open.h"    // this file
#include "modules/process_management/core/scheduler/scheduler.h"              // scheduler_get_current_process
#include "modules/process_management/core/pidfd/pidfd.h"                      // pidfd_create
#include "modules/io_management/core/fd/fd.h"                                 // fd_alloc
#include "modules/process_management/security/pid_reuse_race/defence/pidfd_handle/def_pidfd_handle.h"  // def_pidfd_open_ok

int sys_pidfd_open(uint32_t pid, uint32_t flags) {
    process_t *self = scheduler_get_current_process();
    process_t *t = process_get_by_pid(pid);
    if (!self || flags != 0)
        return -1;
    // DEFENCE pidfd_handle: only a live, non-system process gets a handle.
    if (!def_pidfd_open_ok(t))
        return -1;
    vfs_node_t *node = pidfd_create(t);
    return node ? fd_alloc(self, node, 0) : -1;     // -1: no free pidfd or descriptor
}

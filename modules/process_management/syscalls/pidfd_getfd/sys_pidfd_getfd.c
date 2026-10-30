/*
 * sys_pidfd_getfd.c - pidfd_getfd(pidfd, targetfd, flags).
 *
 * The process named by 'pidfd' has file 'targetfd' open; give the
 * caller a descriptor for the same file (same node, same flags, the
 * offset it had). That is like reading the other process's memory, so it
 * needs ptrace rights (pidfd_handle). Returns the new descriptor or -1.
 *
 * Uses: scheduler_get_current_process (core/scheduler), pidfd_target
 *       (core/pidfd), fd_alloc (io_management core/fd),
 *       def_pidfd_getfd_ok (security/pid_reuse_race/defence/pidfd_handle).
 */
#include "modules/process_management/syscalls/pidfd_getfd/sys_pidfd_getfd.h"  // this file
#include "modules/process_management/core/scheduler/scheduler.h"              // scheduler_get_current_process
#include "modules/process_management/core/pidfd/pidfd.h"                      // pidfd_target, pidfd_is
#include "modules/io_management/core/fd/fd.h"                                 // fd_alloc
#include "modules/process_management/security/pid_reuse_race/defence/pidfd_handle/def_pidfd_handle.h"  // def_pidfd_getfd_ok

int sys_pidfd_getfd(int pidfd, int targetfd, uint32_t flags) {
    process_t *self = scheduler_get_current_process();
    if (!self || flags != 0)
        return -1;
    process_t *t = pidfd_target(self, pidfd);       // ended / not a pidfd: NULL
    if (!t)
        return -1;
    // DEFENCE pidfd_handle: copying a file out of a process needs ptrace rights.
    if (!def_pidfd_getfd_ok(self->uid, t))
        return -1;
    if (targetfd < FIRST_FILE_FD || targetfd >= MAX_FDS || !t->fd_table[targetfd] ||
        pidfd_is(t->fd_table[targetfd]))
        return -1;                                  // EBADF (no handles of handles)

    int fd = fd_alloc(self, t->fd_table[targetfd], t->fd_flags[targetfd]);
    if (fd >= 0)
        self->fd_offset[fd] = t->fd_offset[targetfd];
    return fd;
}

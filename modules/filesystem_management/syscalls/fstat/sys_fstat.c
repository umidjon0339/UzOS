/*
 * sys_fstat.c - fstat(fd, buf): stat the file behind a descriptor.
 *
 * No path, so no permission to walk: having the file open is enough.
 * fds 0, 1, 2 (keyboard, screen) are no files in UzOS: -1.
 *
 * Uses: scheduler_get_current_process (process_management), vfs_stat (core/vfs).
 */
#include "modules/filesystem_management/syscalls/fstat/sys_fstat.h"   // this file
#include "modules/process_management/core/scheduler/scheduler.h"     // scheduler_get_current_process

int sys_fstat(int fd, vfs_stat_t *buf) {
    process_t *self = scheduler_get_current_process();
    if (!self || !buf || fd < 0 || fd >= MAX_FDS || !self->fd_table[fd])
        return -1;                          // EBADF
    vfs_stat(self->fd_table[fd], buf);
    return 0;
}

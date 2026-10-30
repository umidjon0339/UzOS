/*
 * sys_fchdir.c - fchdir(fd): chdir() to a directory you hold open.
 *
 * The descriptor must be a directory (open with O_DIRECTORY or
 * read-only), and you must still be allowed in NOW (search_check): a
 * descriptor opened before a chmod is not a key. Returns 0 or -1.
 *
 * Uses: the current process (process_management), def_search_ok
 *       (security/path_walk_bypass/defence/search_check).
 */
#include "modules/filesystem_management/syscalls/fchdir/sys_fchdir.h"   // this file
#include "modules/process_management/core/scheduler/scheduler.h"      // scheduler_get_current_process
#include "modules/filesystem_management/security/path_walk_bypass/defence/search_check/def_search_check.h"  // def_search_ok

int sys_fchdir(int fd) {
    process_t *self = scheduler_get_current_process();
    if (!self || fd < 0 || fd >= MAX_FDS || !self->fd_table[fd])
        return -1;                                  // EBADF
    vfs_node_t *dir = self->fd_table[fd];
    if (!(dir->flags & FS_DIRECTORY) || !def_search_ok(dir))
        return -1;                                  // ENOTDIR / EACCES
    self->cwd = dir;
    return 0;
}

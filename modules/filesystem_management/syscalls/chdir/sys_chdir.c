/*
 * sys_chdir.c - chdir(path): change the current directory.
 *
 * Each process has its own current directory (process_t.cwd); relative
 * paths like "notes" or "../x" start from there.
 *
 * Entering a directory needs read permission on it (core/perm).
 *
 * Uses: vfs_lookup (core/vfs), the current process (process_management),
 *       perm_check (user_management).
 */

#include "modules/filesystem_management/syscalls/chdir/sys_chdir.h"  // this file
#include "modules/filesystem_management/core/vfs/vfs.h"              // vfs_node_t, vfs_lookup, vfs_cwd
#include "modules/process_management/core/scheduler/scheduler.h"     // scheduler_get_current_process
#include "modules/user_management/core/perm/perm.h"                  // perm_check, PERM_READ

int sys_chdir(const char *path) {
    process_t *self = scheduler_get_current_process();
    vfs_node_t *dir = vfs_lookup(vfs_cwd(), path);

    if (!self || !dir || !(dir->flags & FS_DIRECTORY) || !perm_check(dir, PERM_READ))
        return -1;
    self->cwd = dir;
    return 0;
}

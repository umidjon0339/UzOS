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

#include "sys_chdir.h"
#include "vfs.h"
#include "scheduler.h"
#include "perm.h"

int sys_chdir(const char *path) {
    process_t *self = scheduler_get_current_process();
    vfs_node_t *dir = vfs_lookup(vfs_cwd(), path);

    if (!self || !dir || !(dir->flags & FS_DIRECTORY) || !perm_check(dir, PERM_READ))
        return -1;
    self->cwd = dir;
    return 0;
}

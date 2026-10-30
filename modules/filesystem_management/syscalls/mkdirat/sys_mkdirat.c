/*
 * sys_mkdirat.c - mkdirat(dirfd, path, mode).
 *
 * mkdir(), with a relative path starting at directory 'dirfd'
 * (AT_FDCWD: the current one) and the mode asked for (Linux bits: owner
 * r/w and others r/w are kept, as UzOS has two digits), minus the umask,
 * and never writable by others (secure_umask). Needs write on the
 * parent. Returns 0 or -1.
 *
 * Uses: vfs_start_dir / vfs_lookup_parent (core/vfs), ramdisk_create
 *       (core/ramdisk), perm_check (user_management), the current process's
 *       umask, def_creation_mode (security/umask_weak/defence/secure_umask).
 */
#include "modules/filesystem_management/syscalls/mkdirat/sys_mkdirat.h"   // this file
#include "modules/filesystem_management/core/vfs/vfs.h"                 // vfs_start_dir, vfs_lookup_parent
#include "modules/filesystem_management/core/ramdisk/ramdisk.h"         // ramdisk_create
#include "modules/user_management/core/perm/perm.h"                     // perm_check, PERM_WRITE
#include "modules/process_management/core/scheduler/scheduler.h"        // scheduler_get_current_process
#include "modules/filesystem_management/security/umask_weak/defence/secure_umask/def_secure_umask.h"  // def_creation_mode

int sys_mkdirat(int dirfd, const char *path, uint32_t mode) {
    vfs_node_t *start = vfs_start_dir(dirfd);
    if (!path || (path[0] != '/' && !start))
        return -1;
    char name[FS_NAME_MAX + 1];
    vfs_node_t *dir = vfs_lookup_parent(start, path, name);
    if (!dir || !perm_check(dir, PERM_WRITE))
        return -1;
    vfs_node_t *made = ramdisk_create(dir, name, FS_DIRECTORY);
    if (!made)
        return -1;
    process_t *self = scheduler_get_current_process();
    uint32_t asked = ((mode >> 6) & 6) << 3 | (mode & 6);     // Linux bits -> UzOS digits
    made->mode = def_creation_mode(asked, self ? self->umask : 002);
    return 0;
}

/*
 * sys_symlinkat.c - symlinkat(target, newdirfd, linkpath).
 *
 * symlink(), the link's path relative to directory 'newdirfd'
 * (AT_FDCWD: the current one). The target text is stored as given.
 * Returns 0 or -1.
 *
 * Uses: vfs_start_dir / vfs_lookup_parent (core/vfs), ramdisk_symlink
 *       (core/ramdisk), perm_check (user_management).
 */
#include "modules/filesystem_management/syscalls/symlinkat/sys_symlinkat.h"   // this file
#include "modules/filesystem_management/core/vfs/vfs.h"                     // vfs_start_dir, vfs_lookup_parent
#include "modules/filesystem_management/core/ramdisk/ramdisk.h"             // ramdisk_symlink
#include "modules/user_management/core/perm/perm.h"                         // perm_check, PERM_WRITE

static int make_symlink(const char *target, vfs_node_t *start, const char *linkpath) {
    char name[FS_NAME_MAX + 1];
    vfs_node_t *dir = vfs_lookup_parent(start, linkpath, name);
    if (!dir || !perm_check(dir, PERM_WRITE))
        return -1;
    return ramdisk_symlink(dir, name, target) ? 0 : -1;
}

int sys_symlinkat(const char *target, int newdirfd, const char *linkpath) {
    vfs_node_t *start = vfs_start_dir(newdirfd);
    if (!target || !linkpath || (linkpath[0] != '/' && !start))
        return -1;
    return make_symlink(target, start, linkpath);
}

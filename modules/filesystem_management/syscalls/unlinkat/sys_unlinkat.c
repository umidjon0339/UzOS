/*
 * sys_unlinkat.c - unlinkat(dirfd, path, flags).
 *
 * unlink() of a file, or with AT_REMOVEDIR rmdir() of an empty
 * directory, path relative to directory 'dirfd' (AT_FDCWD: the current
 * one). The wrong kind fails (EISDIR / ENOTDIR). Same rights as unlink:
 * write on the parent and owning the entry; busy_check inside
 * ramdisk_delete. Returns 0 or -1.
 *
 * Uses: vfs_start_dir / vfs_lookup_parent / vfs_finddir (core/vfs),
 *       ramdisk_delete (core/ramdisk), perm_may_remove (user_management).
 */
#include "modules/filesystem_management/syscalls/unlinkat/sys_unlinkat.h"   // this file
#include "modules/filesystem_management/core/vfs/vfs.h"                   // vfs_start_dir, vfs_lookup_parent
#include "modules/filesystem_management/core/ramdisk/ramdisk.h"           // ramdisk_delete
#include "modules/user_management/core/perm/perm.h"                       // perm_may_remove

int sys_unlinkat(int dirfd, const char *path, int flags) {
    vfs_node_t *start = vfs_start_dir(dirfd);
    if (!path || (flags & ~AT_REMOVEDIR) || (path[0] != '/' && !start))
        return -1;
    char name[FS_NAME_MAX + 1];
    vfs_node_t *dir = vfs_lookup_parent(start, path, name);
    vfs_node_t *node = dir ? vfs_finddir(dir, name) : 0;
    if (!node || !perm_may_remove(dir, node))
        return -1;
    int is_dir = (node->flags & FS_DIRECTORY) != 0;
    if (is_dir != ((flags & AT_REMOVEDIR) != 0))
        return -1;                                  // EISDIR / ENOTDIR
    return ramdisk_delete(dir, name);
}

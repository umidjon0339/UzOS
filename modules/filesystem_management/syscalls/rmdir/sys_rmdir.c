/*
 * sys_rmdir.c - rmdir(path): remove an EMPTY directory.
 *
 * Files are unlink()'s job (ENOTDIR here). Needs write permission on the
 * parent and owning the directory (perm_may_remove); busy_check refuses
 * a directory that is not empty, open, or someone's current directory.
 * "." and ".." cannot be removed. Returns 0 or -1.
 *
 * Uses: vfs_lookup_parent / vfs_finddir / vfs_cwd (core/vfs),
 *       ramdisk_delete (core/ramdisk), perm_may_remove (user_management).
 */
#include "modules/filesystem_management/syscalls/rmdir/sys_rmdir.h"     // this file
#include "modules/filesystem_management/core/vfs/vfs.h"               // vfs_lookup_parent, vfs_finddir
#include "modules/filesystem_management/core/ramdisk/ramdisk.h"       // ramdisk_delete
#include "modules/user_management/core/perm/perm.h"                   // perm_may_remove

int sys_rmdir(const char *path) {
    char name[FS_NAME_MAX + 1];
    vfs_node_t *dir = vfs_lookup_parent(vfs_cwd(), path, name);
    vfs_node_t *node = dir ? vfs_finddir(dir, name) : 0;
    if (!node || !(node->flags & FS_DIRECTORY) || !perm_may_remove(dir, node))
        return -1;
    return ramdisk_delete(dir, name);
}

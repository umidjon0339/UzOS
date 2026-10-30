/*
 * sys_unlink.c - unlink(path): delete a file (a directory: rmdir).
 *
 * Refused for a directory (EISDIR, as Linux: use rmdir), and while the
 * file is open (busy_check, in ramdisk_delete).
 *
 * You need write permission on the directory and must own the entry
 * (or be root): perm_may_remove (user_management).
 *
 * Uses: vfs_lookup_parent (core/vfs), ramdisk_delete (core/ramdisk),
 *       perm_may_remove (user_management).
 */

#include "modules/filesystem_management/syscalls/unlink/sys_unlink.h"  // this file
#include "modules/filesystem_management/core/vfs/vfs.h"                // FS_NAME_MAX, vfs_node_t, vfs_lookup_parent
#include "modules/filesystem_management/core/ramdisk/ramdisk.h"        // ramdisk_delete
#include "modules/user_management/core/perm/perm.h"                    // perm_may_remove

int sys_unlink(const char *path) {
    char name[FS_NAME_MAX + 1];
    vfs_node_t *dir = vfs_lookup_parent(vfs_cwd(), path, name);
    vfs_node_t *node = dir ? vfs_finddir(dir, name) : 0;
    if (!node || (node->flags & FS_DIRECTORY) || !perm_may_remove(dir, node))
        return -1;                          // a directory is rmdir()'s job (EISDIR)
    return ramdisk_delete(dir, name);
}

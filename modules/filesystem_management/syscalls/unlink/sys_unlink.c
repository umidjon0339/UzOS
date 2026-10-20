/*
 * sys_unlink.c - unlink(path): delete a file or an empty directory.
 *
 * Refused while the file is open, the directory has entries, or it is
 * some process's current directory (busy_check, in ramdisk_delete).
 *
 * You need write permission on the directory and must own the entry
 * (or be root): perm_may_remove (user_management).
 *
 * Uses: vfs_lookup_parent (core/vfs), ramdisk_delete (core/ramdisk),
 *       perm_may_remove (user_management).
 */

#include "sys_unlink.h"
#include "vfs.h"
#include "ramdisk.h"
#include "perm.h"

int sys_unlink(const char *path) {
    char name[FS_NAME_MAX + 1];
    vfs_node_t *dir = vfs_lookup_parent(vfs_cwd(), path, name);
    vfs_node_t *node = dir ? vfs_finddir(dir, name) : 0;
    if (!node || !perm_may_remove(dir, node))
        return -1;
    return ramdisk_delete(dir, name);
}

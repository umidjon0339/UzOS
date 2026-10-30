/*
 * sys_mkdir.c - mkdir(path): make a new, empty directory.
 *
 * "docs" makes it in the current directory, "/a/b/docs" inside /a/b
 * (which must already exist).
 *
 * You need write permission on the directory it goes in (core/perm).
 *
 * Uses: vfs_lookup_parent (core/vfs), ramdisk_create (core/ramdisk),
 *       perm_check (user_management).
 */

#include "modules/filesystem_management/syscalls/mkdir/sys_mkdir.h"  // this file
#include "modules/filesystem_management/core/vfs/vfs.h"              // FS_NAME_MAX, vfs_node_t, vfs_lookup_parent
#include "modules/filesystem_management/core/ramdisk/ramdisk.h"      // ramdisk_create
#include "modules/user_management/core/perm/perm.h"                  // perm_check, PERM_WRITE

int sys_mkdir(const char *path) {
    char name[FS_NAME_MAX + 1];
    vfs_node_t *dir = vfs_lookup_parent(vfs_cwd(), path, name);
    if (!dir || !perm_check(dir, PERM_WRITE))       // may we add to it?
        return -1;
    return ramdisk_create(dir, name, FS_DIRECTORY) ? 0 : -1;
}

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

#include "sys_mkdir.h"
#include "vfs.h"
#include "ramdisk.h"
#include "perm.h"

int sys_mkdir(const char *path) {
    char name[FS_NAME_MAX + 1];
    vfs_node_t *dir = vfs_lookup_parent(vfs_cwd(), path, name);
    if (!dir || !perm_check(dir, PERM_WRITE))       // may we add to it?
        return -1;
    return ramdisk_create(dir, name, FS_DIRECTORY) ? 0 : -1;
}

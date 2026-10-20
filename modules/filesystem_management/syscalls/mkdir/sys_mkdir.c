/*
 * sys_mkdir.c - mkdir(path): make a new, empty directory.
 *
 * "docs" makes it in the current directory, "/a/b/docs" inside /a/b
 * (which must already exist).
 *
 * Uses: vfs_lookup_parent (core/vfs), ramdisk_create (core/ramdisk).
 */

#include "sys_mkdir.h"
#include "vfs.h"
#include "ramdisk.h"

int sys_mkdir(const char *path) {
    char name[FS_NAME_MAX + 1];
    vfs_node_t *dir = vfs_lookup_parent(vfs_cwd(), path, name);
    return dir && ramdisk_create(dir, name, FS_DIRECTORY) ? 0 : -1;
}

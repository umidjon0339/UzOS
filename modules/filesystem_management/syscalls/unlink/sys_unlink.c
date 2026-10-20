/*
 * sys_unlink.c - unlink(path): delete a file or an empty directory.
 *
 * Refused while the file is open, the directory has entries, or it is
 * some process's current directory (busy_check, in ramdisk_delete).
 *
 * Uses: vfs_lookup_parent (core/vfs), ramdisk_delete (core/ramdisk).
 */

#include "sys_unlink.h"
#include "vfs.h"
#include "ramdisk.h"

int sys_unlink(const char *path) {
    char name[FS_NAME_MAX + 1];
    vfs_node_t *dir = vfs_lookup_parent(vfs_cwd(), path, name);
    return dir ? ramdisk_delete(dir, name) : -1;
}

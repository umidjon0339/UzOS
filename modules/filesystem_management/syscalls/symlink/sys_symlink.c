/*
 * sys_symlink.c - symlink(target, linkpath): a symbolic link.
 *
 * The link stores the TEXT 'target'; it may point anywhere, even to
 * nothing yet. Walking a path through it continues at that text,
 * relative to the link's own directory (vfs_lookup: link_depth,
 * protected_symlinks). Needs write on the link's directory. Returns 0
 * or -1.
 *
 * Uses: vfs_lookup_parent / vfs_cwd (core/vfs), ramdisk_symlink
 *       (core/ramdisk), perm_check (user_management).
 */
#include "modules/filesystem_management/syscalls/symlink/sys_symlink.h"   // this file
#include "modules/filesystem_management/core/vfs/vfs.h"                 // vfs_lookup_parent, vfs_cwd
#include "modules/filesystem_management/core/ramdisk/ramdisk.h"         // ramdisk_symlink
#include "modules/user_management/core/perm/perm.h"                     // perm_check, PERM_WRITE

static int make_symlink(const char *target, vfs_node_t *start, const char *linkpath) {
    char name[FS_NAME_MAX + 1];
    vfs_node_t *dir = vfs_lookup_parent(start, linkpath, name);
    if (!dir || !perm_check(dir, PERM_WRITE))
        return -1;
    return ramdisk_symlink(dir, name, target) ? 0 : -1;
}

int sys_symlink(const char *target, const char *linkpath) {
    if (!target || !linkpath)
        return -1;
    return make_symlink(target, vfs_cwd(), linkpath);
}

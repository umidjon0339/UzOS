/*
 * sys_linkat.c - linkat(olddirfd, old, newdirfd, new, flags).
 *
 * link(), each path relative to its directory descriptor (AT_FDCWD:
 * the current directory). AT_SYMLINK_FOLLOW links the file a final
 * symbolic link points to, instead of refusing the link itself. Same
 * rules: protected_hardlinks, nlink_bounds, write on the new directory.
 * Returns 0 or -1.
 *
 * Uses: vfs_start_dir / vfs_lookup / vfs_lookup_nofollow / vfs_lookup_parent
 *       (core/vfs), ramdisk_link (core/ramdisk), perm_check (user_management),
 *       def_hardlink_ok (security/hardlink_abuse/defence/protected_hardlinks).
 */
#include "modules/filesystem_management/syscalls/linkat/sys_linkat.h"   // this file
#include "modules/filesystem_management/core/vfs/vfs.h"               // vfs_start_dir, vfs_lookup
#include "modules/filesystem_management/core/ramdisk/ramdisk.h"       // ramdisk_link
#include "modules/user_management/core/perm/perm.h"                   // perm_check, PERM_WRITE
#include "modules/filesystem_management/security/hardlink_abuse/defence/protected_hardlinks/def_protected_hardlinks.h"  // def_hardlink_ok

// Make 'new_path' (from 'new_start') another name of 'inode'.
static int make_link(vfs_node_t *inode, vfs_node_t *new_start, const char *new_path) {
    // DEFENCE protected_hardlinks: only your own file, never a directory.
    if (!inode || !def_hardlink_ok(inode))
        return -1;
    char name[FS_NAME_MAX + 1];
    vfs_node_t *dir = vfs_lookup_parent(new_start, new_path, name);
    if (!dir || !perm_check(dir, PERM_WRITE))
        return -1;
    return ramdisk_link(dir, name, inode) ? 0 : -1;     // nlink_bounds inside
}

int sys_linkat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flags) {
    vfs_node_t *from = vfs_start_dir(olddirfd), *to = vfs_start_dir(newdirfd);
    if (!oldpath || !newpath || (flags & ~AT_SYMLINK_FOLLOW) ||
        (oldpath[0] != '/' && !from) || (newpath[0] != '/' && !to))
        return -1;
    vfs_node_t *inode = flags & AT_SYMLINK_FOLLOW ? vfs_lookup(from, oldpath)
                                                  : vfs_lookup_nofollow(from, oldpath);
    return make_link(inode, to, newpath);
}

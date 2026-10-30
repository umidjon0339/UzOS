/*
 * sys_link.c - link(old, new): a hard link - one more NAME for the same file.
 *
 * Both names lead to the same inode: the same bytes, owner, mode, times;
 * `stat` shows Links: 2. Removing one name leaves the file to the other
 * (inode_refcount). Only for a file you own (protected_hardlinks), never
 * a directory; at most 16 names (nlink_bounds); write permission on the
 * new name's directory. A final symbolic link in 'old' is not followed
 * (Linux's link()). Returns 0 or -1.
 *
 * Uses: vfs_lookup_nofollow / vfs_lookup_parent / vfs_cwd (core/vfs),
 *       ramdisk_link (core/ramdisk), perm_check (user_management),
 *       def_hardlink_ok (security/hardlink_abuse/defence/protected_hardlinks).
 */
#include "modules/filesystem_management/syscalls/link/sys_link.h"       // this file
#include "modules/filesystem_management/core/vfs/vfs.h"               // vfs_lookup_nofollow, vfs_lookup_parent
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

int sys_link(const char *oldpath, const char *newpath) {
    if (!oldpath || !newpath)
        return -1;
    return make_link(vfs_lookup_nofollow(vfs_cwd(), oldpath), vfs_cwd(), newpath);
}

/*
 * sys_faccessat2.c - faccessat2(dirfd, path, mode, flags).
 *
 * faccessat() with the flags the old call had no room for:
 * AT_EACCESS (check as the effective user: UzOS has one uid per
 * process, so the same) and AT_SYMLINK_NOFOLLOW (the link itself).
 * Returns 0 or -1.
 *
 * Uses: vfs_start_dir / vfs_lookup / vfs_lookup_nofollow
 *       (core/vfs), perm_check (user_management).
 */
#include "modules/filesystem_management/syscalls/faccessat2/sys_faccessat2.h"   // this file
#include "modules/filesystem_management/core/vfs/vfs.h"                       // vfs_start_dir, vfs_lookup
#include "modules/user_management/core/perm/perm.h"                           // perm_check, PERM_READ, PERM_WRITE

int sys_faccessat2(int dirfd, const char *path, int mode, int flags) {
    vfs_node_t *start = vfs_start_dir(dirfd);
    if (!path || (mode & ~7) || (flags & ~(AT_EACCESS | AT_SYMLINK_NOFOLLOW_A)) ||
        (path[0] != '/' && !start))
        return -1;
    vfs_node_t *node = vfs_lookup(start, path);
    if (flags & AT_SYMLINK_NOFOLLOW_A)
        node = vfs_lookup_nofollow(start, path);
    if (!node)
        return -1;
    if ((mode & 5) && !perm_check(node, PERM_READ))
        return -1;
    if ((mode & 2) && !perm_check(node, PERM_WRITE))
        return -1;
    return 0;
}

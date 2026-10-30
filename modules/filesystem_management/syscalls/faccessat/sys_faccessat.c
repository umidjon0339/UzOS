/*
 * sys_faccessat.c - faccessat(dirfd, path, mode).
 *
 * access(), with a relative path starting at directory 'dirfd'
 * (AT_FDCWD: the current one). Modes: 4 read, 2 write, 1 run/search
 * (UzOS: read), 0 exists. Returns 0 or -1.
 *
 * Uses: vfs_start_dir / vfs_lookup (core/vfs), perm_check (user_management).
 */
#include "modules/filesystem_management/syscalls/faccessat/sys_faccessat.h"   // this file
#include "modules/filesystem_management/core/vfs/vfs.h"                     // vfs_start_dir, vfs_lookup
#include "modules/user_management/core/perm/perm.h"                         // perm_check, PERM_READ, PERM_WRITE

int sys_faccessat(int dirfd, const char *path, int mode) {
    vfs_node_t *start = vfs_start_dir(dirfd);
    if (!path || (mode & ~7) || (path[0] != '/' && !start))
        return -1;
    vfs_node_t *node = vfs_lookup(start, path);
    if (!node)
        return -1;
    if ((mode & 5) && !perm_check(node, PERM_READ))
        return -1;
    if ((mode & 2) && !perm_check(node, PERM_WRITE))
        return -1;
    return 0;
}

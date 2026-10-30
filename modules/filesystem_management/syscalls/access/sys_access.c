/*
 * sys_access.c - access(path, mode): ask before you try.
 *
 * Checks the same rule open() will apply (perm_check: owner digit or
 * others digit, root passes), without opening anything. X_OK: UzOS has
 * no execute bit, so running a program or entering a directory needs
 * READ - X_OK asks for that. Returns 0 if every asked right is there.
 * Careful: the answer may be stale by the time you use it (toctou_race,
 * process_management) - opening and checking the result is safer.
 *
 * Uses: vfs_lookup / vfs_cwd (core/vfs), perm_check (user_management).
 */
#include "modules/filesystem_management/syscalls/access/sys_access.h"   // this file
#include "modules/filesystem_management/core/vfs/vfs.h"               // vfs_lookup, vfs_cwd
#include "modules/user_management/core/perm/perm.h"                   // perm_check, PERM_READ, PERM_WRITE

int sys_access(const char *path, int mode) {
    vfs_node_t *node = vfs_lookup(vfs_cwd(), path);
    if (!node || (mode & ~(R_OK | W_OK | X_OK)))
        return -1;
    if ((mode & (R_OK | X_OK)) && !perm_check(node, PERM_READ))
        return -1;
    if ((mode & W_OK) && !perm_check(node, PERM_WRITE))
        return -1;
    return 0;
}

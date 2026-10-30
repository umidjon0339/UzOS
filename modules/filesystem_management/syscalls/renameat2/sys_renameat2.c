/*
 * sys_renameat2.c - renameat2(olddirfd, old, newdirfd, new, flags).
 *
 *   0                  as renameat()
 *   RENAME_NOREPLACE   fail if 'new' exists: "move unless that would
 *                      overwrite", in one step (no check-then-act race)
 *   RENAME_EXCHANGE    swap the two in one step: an editor saves a file
 *                      by writing a copy and exchanging it with the
 *                      original, so readers see the old or the new one,
 *                      never half
 * Rights: owning BOTH entries (or root) and write on both directories.
 * Returns 0 or -1.
 *
 * Uses: vfs_start_dir / vfs_lookup / vfs_lookup_entry / vfs_lookup_parent (core/vfs),
 *       ramdisk_move / ramdisk_exchange (core/ramdisk), perm_may_remove /
 *       perm_check (user_management).
 */
#include "modules/filesystem_management/syscalls/renameat2/sys_renameat2.h"   // this file
#include "modules/filesystem_management/core/vfs/vfs.h"                     // vfs_start_dir, vfs_lookup
#include "modules/filesystem_management/core/ramdisk/ramdisk.h"             // ramdisk_move, ramdisk_exchange
#include "modules/user_management/core/perm/perm.h"                         // perm_may_remove, perm_check
#include "lib/string/string.h"                                              // strcpy

// Move 'from' (relative to 'from_dir') to 'to' (relative to 'to_dir'),
// the way rename() does: into an existing directory keeps the name.
static int move(vfs_node_t *from_dir, const char *from, vfs_node_t *to_dir, const char *to,
                int no_replace) {
    vfs_node_t *node = vfs_lookup_entry(from_dir, from);   // the name itself: a link moves, not its file
    if (!node || !perm_may_remove(node->parent, node))
        return -1;
    char name[FS_NAME_MAX + 1];
    vfs_node_t *target = vfs_lookup(to_dir, to);
    if (target && no_replace)
        return -1;                                  // EEXIST: a file or a directory
    if (target && (target->flags & FS_DIRECTORY) && target != node) {
        if (!perm_check(target, PERM_WRITE))
            return -1;
        strcpy(name, node->name);
        return ramdisk_move(node, target, name);
    }
    vfs_node_t *dir = vfs_lookup_parent(to_dir, to, name);
    if (!dir || !perm_check(dir, PERM_WRITE))
        return -1;
    return ramdisk_move(node, dir, name);
}

int sys_renameat2(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flags) {
    vfs_node_t *from_dir = vfs_start_dir(olddirfd), *to_dir = vfs_start_dir(newdirfd);
    if (!oldpath || !newpath || (oldpath[0] != '/' && !from_dir) || (newpath[0] != '/' && !to_dir) ||
        (flags & ~(RENAME_NOREPLACE | RENAME_EXCHANGE)) ||
        flags == (RENAME_NOREPLACE | RENAME_EXCHANGE))
        return -1;                                  // EINVAL: the two exclude each other
    if (!(flags & RENAME_EXCHANGE))
        return move(from_dir, oldpath, to_dir, newpath, flags & RENAME_NOREPLACE);

    vfs_node_t *a = vfs_lookup_entry(from_dir, oldpath), *b = vfs_lookup_entry(to_dir, newpath);
    if (!a || !b || !perm_may_remove(a->parent, a) || !perm_may_remove(b->parent, b))
        return -1;
    return ramdisk_exchange(a, b);
}

/*
 * sys_renameat.c - renameat(olddirfd, old, newdirfd, new).
 *
 * rename(), each path relative to its own directory descriptor
 * (AT_FDCWD: the current directory). Same rules: owning the entry and
 * write on both directories; no_cycle, filename_check, path_length,
 * depth_limit inside ramdisk_move. Returns 0 or -1.
 *
 * Uses: vfs_start_dir / vfs_lookup / vfs_lookup_entry / vfs_lookup_parent (core/vfs),
 *       ramdisk_move (core/ramdisk), perm_may_remove / perm_check (user_management).
 */
#include "modules/filesystem_management/syscalls/renameat/sys_renameat.h"   // this file
#include "modules/filesystem_management/core/vfs/vfs.h"                   // vfs_start_dir, vfs_lookup
#include "modules/filesystem_management/core/ramdisk/ramdisk.h"           // ramdisk_move
#include "modules/user_management/core/perm/perm.h"                       // perm_may_remove, perm_check
#include "lib/string/string.h"                                            // strcpy

// Move 'from' (relative to 'from_dir') to 'to' (relative to 'to_dir'),
// the way rename() does: into an existing directory keeps the name.
static int move(vfs_node_t *from_dir, const char *from, vfs_node_t *to_dir, const char *to,
                int no_replace) {
    vfs_node_t *node = vfs_lookup_entry(from_dir, from);   // the name itself: a link moves, not its file
    if (!node || !perm_may_remove(node->parent, node))
        return -1;
    char name[FS_NAME_MAX + 1];
    vfs_node_t *target = vfs_lookup(to_dir, to);
    if (target && no_replace && !(target->flags & FS_DIRECTORY))
        return -1;                                  // EEXIST
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

int sys_renameat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath) {
    vfs_node_t *from_dir = vfs_start_dir(olddirfd), *to_dir = vfs_start_dir(newdirfd);
    if (!oldpath || !newpath || (oldpath[0] != '/' && !from_dir) || (newpath[0] != '/' && !to_dir))
        return -1;
    return move(from_dir, oldpath, to_dir, newpath, 0);
}

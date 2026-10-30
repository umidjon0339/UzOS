/*
 * sys_rename.c - rename(from, to): move or rename an entry.
 *
 *   rename("a", "b")        rename a to b in the same directory
 *   rename("a", "docs")     docs is a directory: move a into it, keep the name
 *   rename("a", "docs/b")   move into docs and call it b
 *
 * Moving takes the entry out of its directory and puts it in another:
 * you must be allowed to remove it from the first (own it, write on the
 * directory) and to write into the second (core/perm).
 *
 * Uses: vfs_lookup / vfs_lookup_entry / vfs_lookup_parent (core/vfs), ramdisk_move
 *       (core/ramdisk), which checks the new name and refuses loops,
 *       perm_may_remove / perm_check (user_management).
 */

#include "modules/filesystem_management/syscalls/rename/sys_rename.h"  // this file
#include "modules/filesystem_management/core/vfs/vfs.h"                // vfs_node_t, vfs_lookup, vfs_cwd
#include "modules/filesystem_management/core/ramdisk/ramdisk.h"        // ramdisk_move
#include "lib/string/string.h"                                         // strcpy
#include "modules/user_management/core/perm/perm.h"                    // perm_may_remove, perm_check, PERM_WRITE

int sys_rename(const char *from, const char *to) {
    vfs_node_t *node = vfs_lookup_entry(vfs_cwd(), from);   // the name itself: a link moves, not its file
    if (!node || !perm_may_remove(node->parent, node))
        return -1;

    char name[FS_NAME_MAX + 1];

    // Moving into an existing directory keeps the old name.
    vfs_node_t *target = vfs_lookup(vfs_cwd(), to);
    if (target && (target->flags & FS_DIRECTORY) && target != node) {
        if (!perm_check(target, PERM_WRITE))
            return -1;
        strcpy(name, node->name);       // a copy: the move rewrites node->name
        return ramdisk_move(node, target, name);
    }

    vfs_node_t *dir = vfs_lookup_parent(vfs_cwd(), to, name);
    if (!dir || !perm_check(dir, PERM_WRITE))
        return -1;
    return ramdisk_move(node, dir, name);
}

/*
 * sys_rename.c - rename(from, to): move or rename an entry.
 *
 *   rename("a", "b")        rename a to b in the same directory
 *   rename("a", "docs")     docs is a directory: move a into it, keep the name
 *   rename("a", "docs/b")   move into docs and call it b
 *
 * Uses: vfs_lookup / vfs_lookup_parent (core/vfs), ramdisk_move
 *       (core/ramdisk), which checks the new name and refuses loops.
 */

#include "sys_rename.h"
#include "vfs.h"
#include "ramdisk.h"
#include "string.h"

int sys_rename(const char *from, const char *to) {
    vfs_node_t *node = vfs_lookup(vfs_cwd(), from);
    if (!node)
        return -1;

    char name[FS_NAME_MAX + 1];

    // Moving into an existing directory keeps the old name.
    vfs_node_t *target = vfs_lookup(vfs_cwd(), to);
    if (target && (target->flags & FS_DIRECTORY) && target != node) {
        strcpy(name, node->name);       // a copy: the move rewrites node->name
        return ramdisk_move(node, target, name);
    }

    vfs_node_t *dir = vfs_lookup_parent(vfs_cwd(), to, name);
    return dir ? ramdisk_move(node, dir, name) : -1;
}

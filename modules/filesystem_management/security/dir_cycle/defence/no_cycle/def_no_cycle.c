/*
 * def_no_cycle.c - DEFENCE no_cycle: the directory tree stays a tree.
 *
 * Stops:  dir_cycle (security/attack/dir_cycle)
 * Called: ramdisk_move() (core/ramdisk), before anything is relinked.
 * Rule:   a directory may not be moved into itself or into any directory
 *         below it.
 * Why:    "mv /a /a/b/c" would make a's parent one of its own children.
 *         The whole subtree is cut off from the root (lost), and code
 *         that walks up to the root, like pwd, would go round forever.
 */

#include "def_no_cycle.h"
#include "registry.h"
#include "audit.h"

int def_move_allowed(vfs_node_t *node, vfs_node_t *new_dir) {
    // Walk up from the destination: meeting 'node' means it is below node.
    if (!((node->flags & FS_DIRECTORY) && vfs_is_ancestor(node, new_dir)))
        return 1;
    audit_log("no_cycle", "move refused: a directory cannot go inside itself");
    return 0;
}

static int selftest(void) {
    // A tiny fake tree:  root -> a -> b
    vfs_node_t root = {0}, a = {0}, b = {0};
    root.flags = a.flags = b.flags = FS_DIRECTORY;
    root.parent = &root;
    a.parent = &root;
    b.parent = &a;

    if (!vfs_is_ancestor(&a, &b)) return 0;     // a is above b
    if (!vfs_is_ancestor(&a, &a)) return 0;     // a node is "above" itself
    if (vfs_is_ancestor(&b, &a)) return 0;      // b is not above a
    if (vfs_is_ancestor(&a, &root)) return 0;   // nor is a above the root
    return 1;
}

static const defence_t defence = {
    "no_cycle",
    "refuse to move a directory into itself or its own subdirectories",
    "dir_cycle",
    selftest
};

void def_no_cycle_register(void) {
    registry_add_defence(&defence);
}

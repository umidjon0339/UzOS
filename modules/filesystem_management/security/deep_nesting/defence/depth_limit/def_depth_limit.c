/*
 * def_depth_limit.c - DEFENCE depth_limit: directories go at most 32 deep.
 *
 * Stops:  deep_nesting (security/deep_nesting)
 * Called: ramdisk_create() and ramdisk_move() (core/ramdisk); the
 *         commands that walk trees (tree, find, du, rm -r) also stop at
 *         FS_DEPTH_MAX.
 * Rule:   no node may sit deeper than FS_DEPTH_MAX levels below /.
 * Why:    `tree`, `find`, `du` and `rm -r` walk a tree by calling
 *         themselves for every directory - each call takes kernel stack,
 *         and a kernel stack is 4 KiB. A user who builds a tree thousands
 *         of levels deep (mkdir x; cd x; repeat) makes the next `find /`
 *         run off its stack and corrupt whatever lies below it. A fixed
 *         depth bounds the stack every walker needs.
 *
 * Uses: audit_log, registry.
 */

#include "modules/filesystem_management/security/deep_nesting/defence/depth_limit/def_depth_limit.h"  // this file
#include "modules/security_management/core/registry/registry.h"     // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"           // audit_log

uint32_t def_depth_of(const vfs_node_t *node) {
    uint32_t depth = 0;
    while (node && node->parent && node->parent != node && depth <= FS_DEPTH_MAX + 1) {
        node = node->parent;
        depth++;
    }
    return depth;
}

static int allowed(uint32_t dir_depth, uint32_t height) {
    return dir_depth + height <= FS_DEPTH_MAX;
}

int def_depth_ok(const vfs_node_t *dir, uint32_t height) {
    if (allowed(def_depth_of(dir), height))
        return 1;
    audit_log("depth_limit", "refused: directories may go 32 levels deep at most");
    return 0;
}

static int selftest(void) {
    if (!allowed(0, 1)) return 0;               // a file in /
    if (!allowed(31, 1)) return 0;              // level 32: fine
    if (allowed(32, 1)) return 0;               // level 33: refused
    if (allowed(20, 13)) return 0;              // a tree moved in too deep
    return 1;
}

static const defence_t defence = {
    "depth_limit",
    "directories nest at most 32 deep: tree walkers fit their stacks",
    "deep_nesting",
    selftest
};

void def_depth_limit_register(void) { registry_add_defence(&defence); }

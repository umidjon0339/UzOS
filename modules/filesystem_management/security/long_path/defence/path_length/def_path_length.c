/*
 * def_path_length.c - DEFENCE path_length: every node has a printable path.
 *
 * Stops:  long_path (security/long_path)
 * Called: ramdisk_create() and ramdisk_move() (core/ramdisk), before a
 *         name is added somewhere in the tree.
 * Rule:   the full path of every node (and of everything below a moved
 *         directory) stays shorter than FS_PATH_MAX (256) bytes.
 * Why:    each name may be up to 127 bytes, and nesting has no limit of
 *         its own: build a/b/c/... deep enough and some node's path no
 *         longer fits any path buffer. getcwd() then fails, `pwd` and the
 *         prompt cannot show where you are, `rm -r` from above cannot
 *         name it, and every kernel function that builds a path must be
 *         perfect about truncation. Refusing to create such a node keeps
 *         the rule "every path fits" true everywhere.
 *
 * Uses: vfs_path_of (core/vfs), strlen, audit_log, registry.
 */

#include "modules/filesystem_management/security/long_path/defence/path_length/def_path_length.h"  // this file
#include "modules/security_management/core/registry/registry.h"     // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"           // audit_log
#include "lib/string/string.h"                                      // strlen

static int fits(uint32_t dir_len, uint32_t name_len, uint32_t below) {
    // "/dir" + "/" + "name" + the deepest path below + NUL
    return dir_len + 1 + name_len + below + 1 <= FS_PATH_MAX;
}

int def_path_fits(const vfs_node_t *dir, const char *name, uint32_t below) {
    char path[FS_PATH_MAX];
    if (dir && vfs_path_of((vfs_node_t *)dir, path, sizeof(path)) == 0 &&
        fits(strlen(path), strlen(name), below))
        return 1;
    audit_log("path_length", "refused: the full path would be longer than 255 bytes");
    return 0;
}

static int selftest(void) {
    if (!fits(1, 10, 0)) return 0;              // "/" + "x..." : fine
    if (!fits(200, 50, 0)) return 0;            // 252 bytes: fits
    if (fits(200, 60, 0)) return 0;             // 262: refused
    if (fits(100, 10, 200)) return 0;           // a moved tree that would overflow
    return 1;
}

static const defence_t defence = {
    "path_length",
    "no node may get a full path longer than 255 bytes",
    "long_path",
    selftest
};

void def_path_length_register(void) { registry_add_defence(&defence); }

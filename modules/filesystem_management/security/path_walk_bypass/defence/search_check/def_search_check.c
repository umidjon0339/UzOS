/*
 * def_search_check.c - DEFENCE search_check: no shortcut through a closed
 * directory.
 *
 * Stops:  path_walk_bypass (security/path_walk_bypass)
 * Called: vfs_lookup() and vfs_lookup_parent() (core/vfs), for EVERY
 *         directory a path passes through - the first one too, whether it
 *         is the root, the current directory, or a "*at" call's dirfd.
 * Rule:   to pass through a directory you need read permission on it
 *         (UzOS has no x bit; read stands for "search"). That includes
 *         '..' out of it and a dirfd opened before its mode changed.
 * Why:    a private directory (mode 60) hides everything below it - but
 *         only if EVERY way of reaching its files checks it: the full
 *         path, a path that goes in and back out with "..", and a path
 *         relative to a directory descriptor opened earlier. One path
 *         that skips the check and the whole subtree is open.
 *
 * Uses: perm_check (user_management), audit_log, registry.
 */

#include "modules/filesystem_management/security/path_walk_bypass/defence/search_check/def_search_check.h"  // this file
#include "modules/user_management/core/perm/perm.h"                 // perm_check, PERM_READ
#include "modules/security_management/core/registry/registry.h"     // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"           // audit_log

static int passable(const vfs_node_t *dir, int may_read) {
    return dir && (dir->flags & FS_DIRECTORY) && may_read;
}

int def_search_ok(const vfs_node_t *dir) {
    if (passable(dir, dir && perm_check(dir, PERM_READ)))
        return 1;
    audit_log("search_check", "path walk refused: no right to pass through a directory");
    return 0;
}

static int selftest(void) {
    vfs_node_t dir = {0}, file = {0};
    dir.flags = FS_DIRECTORY;
    file.flags = FS_FILE;
    if (!passable(&dir, 1)) return 0;       // a readable directory
    if (passable(&dir, 0)) return 0;        // closed to us: no way through
    if (passable(&file, 1)) return 0;       // a file has nothing to pass into
    if (passable(0, 1)) return 0;
    return 1;
}

static const defence_t defence = {
    "search_check",
    "every directory on a path (and a dirfd) needs read to pass through",
    "path_walk_bypass",
    selftest
};

void def_search_check_register(void) { registry_add_defence(&defence); }

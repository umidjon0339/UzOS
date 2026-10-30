/*
 * def_protected_hardlinks.c - DEFENCE protected_hardlinks: you name only
 * your own files.
 *
 * Stops:  hardlink_abuse (security/hardlink_abuse)
 * Called: sys_link() and sys_linkat() (syscalls/), before the new name
 *         is made.
 * Rules:  1. a hard link only to a file the caller owns (or CAP_FILES)
 *         2. never to a directory (it would make loops in the tree)
 *         (Linux: fs.protected_hardlinks = 1.)
 * Why:    a hard link is a second name for the SAME file. A user who links
 *         root's file into their home keeps it alive after root deletes
 *         it (an old password list, a vulnerable program root thought was
 *         gone), and can wait for root to later chmod or chown "their"
 *         name - changing root's file. They never needed rights on it.
 *
 * Uses: process_caller_uid (process_management), caps_has / CAP_FILES
 *       (security_management), audit_log, registry.
 */
#include "modules/filesystem_management/security/hardlink_abuse/defence/protected_hardlinks/def_protected_hardlinks.h"  // this file
#include "modules/process_management/core/process/process.h"        // process_caller_uid
#include "modules/security_management/core/caps/caps.h"             // caps_has, CAP_FILES
#include "modules/security_management/core/registry/registry.h"     // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"           // audit_log

static int allowed(int is_dir, int is_owner, int has_cap) {
    return !is_dir && (is_owner || has_cap);
}

int def_hardlink_ok(const vfs_node_t *inode) {
    uint32_t uid = process_caller_uid();
    if (inode && allowed((inode->flags & FS_DIRECTORY) != 0, inode->owner == uid, caps_has(uid, CAP_FILES)))
        return 1;
    audit_log("protected_hardlinks", "hard link refused: not your file, or a directory");
    return 0;
}

static int selftest(void) {
    if (!allowed(0, 1, 0)) return 0;            // my own file
    if (allowed(0, 0, 0)) return 0;             // someone else's
    if (!allowed(0, 0, 1)) return 0;            // CAP_FILES
    if (allowed(1, 1, 1)) return 0;             // a directory: never
    return 1;
}

static const defence_t defence = {
    "protected_hardlinks",
    "hard links only to your own files, never to directories",
    "hardlink_abuse",
    selftest
};

void def_protected_hardlinks_register(void) { registry_add_defence(&defence); }

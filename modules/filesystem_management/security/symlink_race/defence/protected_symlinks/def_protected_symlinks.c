/*
 * def_protected_symlinks.c - DEFENCE protected_symlinks: no traps in shared
 * directories.
 *
 * Stops:  symlink_race (security/symlink_race)
 * Called: vfs_lookup (core/vfs), before it follows a symbolic link.
 * Rule:   a link in a directory others may write to is followed only if
 *         the follower owns the link, or the directory's owner does.
 *         (Linux: fs.protected_symlinks = 1.)
 * Why:    a root program writes "/shared/report". Before it does, a user
 *         plants /shared/report -> /etc/passwd. Following the link, root
 *         overwrites the password file with the report. The user could
 *         not write /etc/passwd - root did it for them. Refusing to
 *         follow a stranger's link in a shared directory closes the trap
 *         (the classic /tmp race).
 *
 * Root is held to the rule too: it is the usual victim.
 *
 * Uses: process_caller_uid (process_management), audit_log, registry.
 */
#include "modules/filesystem_management/security/symlink_race/defence/protected_symlinks/def_protected_symlinks.h"  // this file
#include "modules/process_management/core/process/process.h"        // process_caller_uid
#include "modules/security_management/core/registry/registry.h"     // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"           // audit_log

#define OTHERS_WRITE 002

static int allowed(int dir_shared, uint32_t follower, uint32_t link_owner, uint32_t dir_owner) {
    if (!dir_shared)
        return 1;                               // only the owner could have put it there
    return follower == link_owner || dir_owner == link_owner;
}

int def_follow_ok(const vfs_node_t *dir, const vfs_node_t *link) {
    if (!dir || allowed((dir->mode & OTHERS_WRITE) != 0, process_caller_uid(), link->owner, dir->owner))
        return 1;
    audit_log("protected_symlinks", "not following another user's link in a shared directory");
    return 0;
}

static int selftest(void) {
    if (!allowed(0, 0, 1, 0)) return 0;         // a private directory: always
    if (allowed(1, 0, 1, 0)) return 0;          // root, user 1's link in root's shared dir: no
    if (!allowed(1, 1, 1, 0)) return 0;         // user 1 follows its own link
    if (!allowed(1, 2, 0, 0)) return 0;         // the directory owner's link: fine
    return 1;
}

static const defence_t defence = {
    "protected_symlinks",
    "in a shared directory, follow only your own or the owner's links",
    "symlink_race",
    selftest
};

void def_protected_symlinks_register(void) { registry_add_defence(&defence); }

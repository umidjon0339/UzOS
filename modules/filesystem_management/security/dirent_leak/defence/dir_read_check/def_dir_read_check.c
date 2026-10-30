/*
 * def_dir_read_check.c - DEFENCE dir_read_check: listing a directory needs
 * read permission, every time.
 *
 * Stops:  dirent_leak (security/dirent_leak)
 * Called: sys_getdents / sys_getdents64 (syscalls/), before entries are
 *         copied out.
 * Rule:   the caller may read the directory NOW (perm_check). Linux checks
 *         only at open(); UzOS checks again at every getdents.
 * Why:    a descriptor lives on after the rights it was opened with: the
 *         owner runs `chmod 60` on a directory, but a program that opened
 *         it a minute earlier still holds a descriptor - and file names
 *         alone leak a lot (who has mail, which project exists, what a
 *         backup is called). Checking at every listing closes that window.
 *
 * Uses: perm_check (user_management), audit_log, registry.
 */

#include "modules/filesystem_management/security/dirent_leak/defence/dir_read_check/def_dir_read_check.h"  // this file
#include "modules/user_management/core/perm/perm.h"                 // perm_check, PERM_READ
#include "modules/security_management/core/registry/registry.h"     // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"           // audit_log

static int listable(const vfs_node_t *dir, int may_read_now) {
    return dir && (dir->flags & FS_DIRECTORY) && may_read_now;
}

int def_dir_list_ok(const vfs_node_t *dir) {
    if (listable(dir, dir && perm_check(dir, PERM_READ)))
        return 1;
    audit_log("dir_read_check", "directory listing refused: no read permission now");
    return 0;
}

static int selftest(void) {
    vfs_node_t dir = {0}, file = {0};
    dir.flags = FS_DIRECTORY;
    file.flags = FS_FILE;
    if (!listable(&dir, 1)) return 0;       // readable now: listed
    if (listable(&dir, 0)) return 0;        // chmod'ed since open: refused
    if (listable(&file, 1)) return 0;       // a file has no entries
    return 1;
}

static const defence_t defence = {
    "dir_read_check",
    "getdents re-checks read permission at every listing",
    "dirent_leak",
    selftest
};

void def_dir_read_check_register(void) { registry_add_defence(&defence); }

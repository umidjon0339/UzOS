/*
 * sys_fchownat.c - fchownat(dirfd, path, uid, gid, flags): chown relative to a directory.
 *
 * Only CAP_CHOWN (root) may give a file away, and only to a user that
 * exists; uid -1 keeps the owner. The gid is ignored: UzOS has no
 * groups. ctime becomes now. Returns 0 or -1.
 *
 * Uses: vfs_lookup / vfs_lookup_nofollow / vfs_cwd /
 *       vfs_start_dir / vfs_now (core/vfs), caps_caller_has (security_management),
 *       users_find_uid (user_management), the current process.
 */
#include "modules/filesystem_management/syscalls/fchownat/sys_fchownat.h"   // this file
#include "modules/filesystem_management/core/vfs/vfs.h"               // vfs_lookup, vfs_now
#include "modules/security_management/core/caps/caps.h"               // caps_caller_has, CAP_CHOWN
#include "modules/user_management/core/users/users.h"                 // users_find_uid
#include "modules/process_management/core/scheduler/scheduler.h"      // scheduler_get_current_process

// Give 'node' to user 'uid' (-1: keep). Only CAP_CHOWN (root) may: a
// user giving files away could dodge a quota or frame someone else.
// 'gid' is accepted and ignored: UzOS has no groups.
static int change_owner(vfs_node_t *node, uint32_t uid, uint32_t gid) {
    (void)gid;
    if (!node)
        return -1;
    if (uid == (uint32_t)-1)
        return 0;                               // nothing to change
    if (!caps_caller_has(CAP_CHOWN) || !users_find_uid(uid))
        return -1;
    node->owner = uid;
    node->ctime = vfs_now();
    return 0;
}

int sys_fchownat(int dirfd, const char *path, uint32_t uid, uint32_t gid, int flags) {
    vfs_node_t *start = vfs_start_dir(dirfd);
    if (!path || (flags & ~0x100) || (path[0] != '/' && !start))
        return -1;
    vfs_node_t *node = vfs_lookup(start, path);
    if (flags & 0x100)
        node = vfs_lookup_nofollow(start, path);
    return change_owner(node, uid, gid);
}

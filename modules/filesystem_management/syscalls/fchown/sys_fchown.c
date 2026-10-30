/*
 * sys_fchown.c - fchown(fd, uid, gid): chown the file behind a descriptor.
 *
 * Only CAP_CHOWN (root) may give a file away, and only to a user that
 * exists; uid -1 keeps the owner. The gid is ignored: UzOS has no
 * groups. ctime becomes now. Returns 0 or -1.
 *
 * Uses: vfs_lookup / vfs_lookup_parent / vfs_finddir / vfs_cwd /
 *       vfs_start_dir / vfs_now (core/vfs), caps_caller_has (security_management),
 *       users_find_uid (user_management), the current process.
 */
#include "modules/filesystem_management/syscalls/fchown/sys_fchown.h"   // this file
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

int sys_fchown(int fd, uint32_t uid, uint32_t gid) {
    process_t *self = scheduler_get_current_process();
    if (!self || fd < 0 || fd >= MAX_FDS)
        return -1;
    return change_owner(self->fd_table[fd], uid, gid);
}

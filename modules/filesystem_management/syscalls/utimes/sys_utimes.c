/*
 * sys_utimes.c - utimes(path, tv[2]): tv[0] = access, tv[1] = change.
 *
 * Like utime(), with microseconds that UzOS drops (its times are whole
 * seconds). tv = 0: now. time_owner decides. Returns 0 or -1.
 *
 * Uses: vfs_lookup / vfs_cwd / vfs_now (core/vfs), def_utime_ok
 *       (security/timestamp_forgery/defence/time_owner).
 */
#include "modules/filesystem_management/syscalls/utimes/sys_utimes.h"   // this file
#include "modules/filesystem_management/core/vfs/vfs.h"               // vfs_lookup, vfs_now
#include "modules/filesystem_management/security/timestamp_forgery/defence/time_owner/def_time_owner.h"  // def_utime_ok

// Set 'node's times: 'explicit' = chosen values (owner only), else now
// (owner or a writer). DEFENCE time_owner decides; ctime is always now.
static int set_times(vfs_node_t *node, int explicit_times, uint32_t atime, uint32_t mtime) {
    if (!node || !def_utime_ok(node, explicit_times))
        return -1;
    uint32_t now = vfs_now();
    node->atime = explicit_times ? atime : now;
    node->mtime = explicit_times ? mtime : now;
    node->ctime = now;
    return 0;
}

int sys_utimes(const char *path, const utimes_timeval_t tv[2]) {
    vfs_node_t *node = vfs_lookup(vfs_cwd(), path);
    if (tv && (tv[0].usec >= 1000000 || tv[1].usec >= 1000000))
        return -1;                                  // EINVAL
    return set_times(node, tv != 0, tv ? tv[0].sec : 0, tv ? tv[1].sec : 0);
}

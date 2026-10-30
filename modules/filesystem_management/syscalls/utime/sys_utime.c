/*
 * sys_utime.c - utime(path, times): the oldest of the three; whole seconds.
 *
 * times = 0: both become now (owner or anyone who may write it);
 * else they become the given seconds (owner only): time_owner.
 * Returns 0 or -1.
 *
 * Uses: vfs_lookup / vfs_cwd / vfs_now (core/vfs), def_utime_ok
 *       (security/timestamp_forgery/defence/time_owner).
 */
#include "modules/filesystem_management/syscalls/utime/sys_utime.h"   // this file
#include "modules/filesystem_management/core/vfs/vfs.h"             // vfs_lookup, vfs_now
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

int sys_utime(const char *path, const utimbuf_t *times) {
    vfs_node_t *node = vfs_lookup(vfs_cwd(), path);
    return set_times(node, times != 0, times ? times->actime : 0, times ? times->modtime : 0);
}

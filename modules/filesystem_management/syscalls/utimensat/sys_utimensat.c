/*
 * sys_utimensat.c - utimensat(dirfd, path, ts[2], flags).
 *
 * The modern one (what `touch` uses): ts[0] = access, ts[1] = change;
 * each may say UTIME_NOW or UTIME_OMIT in its nsec. ts = 0: both now.
 * Only "now" for both counts as the harmless case (time_owner: a writer
 * may); any chosen time needs the owner. path relative to dirfd
 * (AT_FDCWD: the current directory). Returns 0 or -1.
 *
 * Uses: vfs_start_dir / vfs_lookup / vfs_now (core/vfs), def_utime_ok
 *       (security/timestamp_forgery/defence/time_owner).
 */
#include "modules/filesystem_management/syscalls/utimensat/sys_utimensat.h"   // this file
#include "modules/filesystem_management/core/vfs/vfs.h"                     // vfs_start_dir, vfs_lookup, vfs_now
#include "modules/filesystem_management/security/timestamp_forgery/defence/time_owner/def_time_owner.h"  // def_utime_ok

static int valid(const utimensat_ts_t *t) {
    return t->nsec < 1000000000u || t->nsec == UTIME_NOW || t->nsec == UTIME_OMIT;
}

int sys_utimensat(int dirfd, const char *path, const utimensat_ts_t ts[2], int flags) {
    vfs_node_t *start = vfs_start_dir(dirfd);
    if (!path || (flags & ~0x100) || (path[0] != '/' && !start) ||
        (ts && (!valid(&ts[0]) || !valid(&ts[1]))))
        return -1;
    vfs_node_t *node = vfs_lookup(start, path);
    if (!node)
        return -1;

    int chosen = ts && ((ts[0].nsec != UTIME_NOW && ts[0].nsec != UTIME_OMIT) ||
                        (ts[1].nsec != UTIME_NOW && ts[1].nsec != UTIME_OMIT));
    if (!def_utime_ok(node, chosen))
        return -1;
    uint32_t now = vfs_now();
    if (!ts || ts[0].nsec != UTIME_OMIT)
        node->atime = !ts || ts[0].nsec == UTIME_NOW ? now : ts[0].sec;
    if (!ts || ts[1].nsec != UTIME_OMIT)
        node->mtime = !ts || ts[1].nsec == UTIME_NOW ? now : ts[1].sec;
    node->ctime = now;
    return 0;
}

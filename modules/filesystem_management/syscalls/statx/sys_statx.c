/*
 * sys_statx.c - statx(dirfd, path, flags, mask, buf).
 *
 * Linux's newer stat: the caller says which fields it wants ('mask'),
 * the kernel answers which ones it filled (buf->mask) - so a program can
 * ask for the size without making a network filesystem fetch the times.
 * UzOS fills them all every time (STATX_BASIC_STATS). dirfd and flags
 * as fstatat. Returns 0 or -1.
 *
 * Uses: vfs_start_dir / vfs_lookup / vfs_stat (core/vfs).
 */
#include "modules/filesystem_management/syscalls/statx/sys_statx.h"   // this file
#include "modules/filesystem_management/core/vfs/vfs.h"             // vfs_lookup, vfs_stat, vfs_start_dir
#include "lib/string/string.h"                                      // memset

#define EMPTY_PATH 0x1000

int sys_statx(int dirfd, const char *path, int flags, uint32_t mask, statx_t *buf) {
    (void)mask;                                     // UzOS always fills everything
    if (!path || !buf)
        return -1;
    vfs_node_t *start = vfs_start_dir(dirfd);
    vfs_node_t *node = !path[0] && (flags & EMPTY_PATH) ? start
                     : (path[0] == '/' || start) ? vfs_lookup(start, path) : 0;
    if (!node)
        return -1;

    vfs_stat_t st;
    vfs_stat(node, &st);
    memset(buf, 0, sizeof(*buf));
    buf->mask    = STATX_BASIC_STATS;
    buf->blksize = st.blksize;
    buf->nlink   = st.nlink;
    buf->uid     = st.uid;
    buf->gid     = st.gid;
    buf->mode    = st.mode;
    buf->ino     = st.ino;
    buf->size    = st.size;
    buf->blocks  = st.blocks;
    buf->atime_sec = st.atime;
    buf->mtime_sec = st.mtime;
    buf->ctime_sec = st.ctime;
    // btime stays 0 and STATX_BTIME is not in mask: UzOS keeps no birth time
    buf->dev     = st.dev;
    return 0;
}

/*
 * sys_readlinkat.c - readlinkat(dirfd, path, buf, size).
 *
 * readlink(), the path relative to directory 'dirfd' (AT_FDCWD: the
 * current one). Returns the count of bytes, or -1.
 *
 * Uses: vfs_start_dir / vfs_lookup_nofollow (core/vfs), memcpy (lib/string).
 */
#include "modules/filesystem_management/syscalls/readlinkat/sys_readlinkat.h"   // this file
#include "modules/filesystem_management/core/vfs/vfs.h"                       // vfs_start_dir, vfs_lookup_nofollow
#include "lib/string/string.h"                                                // memcpy

// Copy the target text of the link 'path' names (not followed) into
// 'buf', at most 'size' bytes and WITHOUT a NUL, as Linux. The count, or -1.
static int read_link(vfs_node_t *start, const char *path, char *buf, uint32_t size) {
    vfs_node_t *link = vfs_lookup_nofollow(start, path);
    if (!link || !(link->flags & FS_LINK) || !buf || size == 0)
        return -1;                                  // EINVAL: not a symbolic link
    uint32_t n = link->size < size ? link->size : size;
    memcpy(buf, link->data, n);
    return (int)n;
}

int sys_readlinkat(int dirfd, const char *path, char *buf, uint32_t size) {
    vfs_node_t *start = vfs_start_dir(dirfd);
    if (!path || (path[0] != '/' && !start))
        return -1;
    return read_link(start, path, buf, size);
}

/*
 * sys_readlink.c - readlink(path, buf, size): the text a symbolic link holds.
 *
 * The link itself is read, not followed. The text is copied without a
 * terminating NUL (as Linux): the count returned says how long it is.
 * -1 if 'path' is not a symbolic link.
 *
 * Uses: vfs_lookup_nofollow / vfs_cwd (core/vfs), memcpy (lib/string).
 */
#include "modules/filesystem_management/syscalls/readlink/sys_readlink.h"   // this file
#include "modules/filesystem_management/core/vfs/vfs.h"                   // vfs_lookup_nofollow, vfs_cwd
#include "lib/string/string.h"                                            // memcpy

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

int sys_readlink(const char *path, char *buf, uint32_t size) {
    return path ? read_link(vfs_cwd(), path, buf, size) : -1;
}

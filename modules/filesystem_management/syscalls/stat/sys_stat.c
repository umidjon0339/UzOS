/*
 * sys_stat.c - stat(path, buf): a file's inode, without opening it.
 *
 * Type and permission bits, inode number, link count, owner, size,
 * blocks and the three times (vfs_stat, core/vfs). Needs only the right
 * to walk the path (read on every directory on the way: vfs_lookup),
 * not to read the file itself - as in Linux. A symbolic link is
 * followed (lstat does not). Returns 0 or -1.
 *
 * Uses: vfs_lookup / vfs_cwd / vfs_stat (core/vfs).
 */
#include "modules/filesystem_management/syscalls/stat/sys_stat.h"   // this file

int sys_stat(const char *path, vfs_stat_t *buf) {
    vfs_node_t *node = vfs_lookup(vfs_cwd(), path);
    if (!node || !buf)
        return -1;
    vfs_stat(node, buf);
    return 0;
}

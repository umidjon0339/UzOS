/*
 * sys_lstat.c - lstat(path, buf): like stat(), but if the LAST name is a
 * symbolic link, describe the link itself instead of what it points to.
 * Returns 0 or -1.
 *
 * Uses: vfs_lookup_nofollow / vfs_cwd / vfs_stat (core/vfs).
 */
#include "modules/filesystem_management/syscalls/lstat/sys_lstat.h"   // this file

int sys_lstat(const char *path, vfs_stat_t *buf) {
    vfs_node_t *node = vfs_lookup_nofollow(vfs_cwd(), path);
    if (!node || !buf)
        return -1;
    vfs_stat(node, buf);
    return 0;
}

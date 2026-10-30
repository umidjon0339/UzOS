/*
 * sys_fstatat.c - fstatat(dirfd, path, buf, flags) ("newfstatat").
 *
 * stat(), with a relative path starting at the directory open as
 * 'dirfd' (or AT_FDCWD: the current directory); an absolute path
 * ignores dirfd. AT_EMPTY_PATH with "" describes dirfd itself;
 * AT_SYMLINK_NOFOLLOW describes a final symbolic link instead of its
 * target. Returns 0 or -1.
 *
 * Uses: vfs_start_dir / vfs_lookup / vfs_lookup_nofollow /
 *       vfs_stat (core/vfs).
 */
#include "modules/filesystem_management/syscalls/fstatat/sys_fstatat.h"   // this file

int sys_fstatat(int dirfd, const char *path, vfs_stat_t *buf, int flags) {
    if (!path || !buf || (flags & ~(AT_SYMLINK_NOFOLLOW | AT_EMPTY_PATH)))
        return -1;
    vfs_node_t *start = vfs_start_dir(dirfd);
    if (!path[0]) {                                 // AT_EMPTY_PATH: dirfd itself
        if (!(flags & AT_EMPTY_PATH) || !start)
            return -1;
        vfs_stat(start, buf);
        return 0;
    }
    if (path[0] != '/' && !start)
        return -1;                                  // EBADF / ENOTDIR

    vfs_node_t *node = vfs_lookup(start, path);
    if (flags & AT_SYMLINK_NOFOLLOW)
        node = vfs_lookup_nofollow(start, path);
    if (!node)
        return -1;
    vfs_stat(node, buf);
    return 0;
}

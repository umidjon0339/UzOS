/*
 * sys_getcwd.c - getcwd(buf, size): write the current directory's path.
 *
 * The VFS only stores names and parent links, so the path is built by
 * walking up from the directory to the root.
 *
 * Uses: vfs_cwd / vfs_path_of (core/vfs).
 */

#include "modules/filesystem_management/syscalls/getcwd/sys_getcwd.h"  // this file
#include "modules/filesystem_management/core/vfs/vfs.h"                // vfs_path_of, vfs_cwd

int sys_getcwd(char *buf, uint32_t size) {
    if (!buf)
        return -1;
    return vfs_path_of(vfs_cwd(), buf, size);
}

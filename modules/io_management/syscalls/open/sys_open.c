/*
 * sys_open.c - open(path, flags): find (or create) a file, give back a number.
 *
 *   open("notes", O_READ)                        read an existing file
 *   open("/docs/log", O_WRITE|O_CREATE|O_APPEND) add to the end, create if new
 *   open("out", O_WRITE|O_CREATE|O_TRUNC)        start the file from empty
 *
 * Directories cannot be opened like files.
 *
 * Uses: vfs_lookup / vfs_lookup_parent (filesystem_management core/vfs),
 *       ramdisk_create / ramdisk_truncate (core/ramdisk), fd_alloc (core/fd).
 */

#include "sys_open.h"
#include "fd.h"
#include "ramdisk.h"
#include "scheduler.h"

int sys_open(const char *path, uint32_t flags) {
    process_t *self = scheduler_get_current_process();
    if (!self || !path || !(flags & (O_READ | O_WRITE)))
        return -1;                          // must want to read or write

    vfs_node_t *file = vfs_lookup(vfs_cwd(), path);
    if (!file && (flags & O_CREATE)) {
        char name[FS_NAME_MAX + 1];
        vfs_node_t *dir = vfs_lookup_parent(vfs_cwd(), path, name);
        file = dir ? ramdisk_create(dir, name, FS_FILE) : 0;
    }
    if (!file || !(file->flags & FS_FILE))
        return -1;                          // missing, or a directory

    int fd = fd_alloc(self, file, flags);
    if (fd >= 0 && (flags & O_TRUNC) && (flags & O_WRITE))
        ramdisk_truncate(file);
    return fd;
}

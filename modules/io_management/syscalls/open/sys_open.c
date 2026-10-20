/*
 * sys_open.c - open(path, flags): find (or create) a file, give back a number.
 *
 *   open("notes", O_READ)                        read an existing file
 *   open("/docs/log", O_WRITE|O_CREATE|O_APPEND) add to the end, create if new
 *   open("out", O_WRITE|O_CREATE|O_TRUNC)        start the file from empty
 *
 * Directories cannot be opened like files. Reading needs read
 * permission on the file, writing needs write permission, and creating
 * needs write permission on its directory (user_management core/perm).
 *
 * Uses: vfs_lookup / vfs_lookup_parent (filesystem_management core/vfs),
 *       ramdisk_create / ramdisk_truncate (core/ramdisk), fd_alloc (core/fd),
 *       perm_check (user_management).
 */

#include "sys_open.h"
#include "fd.h"
#include "ramdisk.h"
#include "scheduler.h"
#include "perm.h"

int sys_open(const char *path, uint32_t flags) {
    process_t *self = scheduler_get_current_process();
    if (!self || !path || !(flags & (O_READ | O_WRITE)))
        return -1;                          // must want to read or write

    vfs_node_t *file = vfs_lookup(vfs_cwd(), path);
    if (!file && (flags & O_CREATE)) {
        char name[FS_NAME_MAX + 1];
        vfs_node_t *dir = vfs_lookup_parent(vfs_cwd(), path, name);
        if (!dir || !perm_check(dir, PERM_WRITE))
            return -1;                      // may not add files there
        file = ramdisk_create(dir, name, FS_FILE);
    }
    if (!file || !(file->flags & FS_FILE))
        return -1;                          // missing, or a directory

    int want = (flags & O_READ ? PERM_READ : 0) | (flags & O_WRITE ? PERM_WRITE : 0);
    if (!perm_check(file, want))
        return -1;                          // the file's mode says no

    int fd = fd_alloc(self, file, flags);
    if (fd >= 0 && (flags & O_TRUNC) && (flags & O_WRITE))
        ramdisk_truncate(file);
    return fd;
}

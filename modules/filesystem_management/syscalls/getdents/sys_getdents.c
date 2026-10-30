/*
 * sys_getdents.c - getdents(fd, buf, count): the entries of a directory.
 *
 * 'fd' is a directory opened read-only. Its descriptor offset counts
 * entries: 0 is ".", 1 is "..", then the directory's own entries. Each
 * call fills 'buf' with as many whole records as fit and moves the
 * offset on; 0 = the end, -1 = an error (not a directory, 'count' too
 * small for one record, or no read permission NOW: dir_read_check).
 * This is what `ls` in Linux calls under the hood (32-bit records).
 *
 * Uses: the current process (process_management), vfs_readdir (core/vfs),
 *       memcpy / strlen (lib/string), def_dir_list_ok
 *       (security/dirent_leak/defence/dir_read_check).
 */
#include "modules/filesystem_management/syscalls/getdents/sys_getdents.h"   // this file
#include "modules/process_management/core/scheduler/scheduler.h"       // scheduler_get_current_process
#include "modules/filesystem_management/core/vfs/vfs.h"                // vfs_readdir
#include "lib/string/string.h"                                         // memcpy, strlen
#include "modules/filesystem_management/security/dirent_leak/defence/dir_read_check/def_dir_read_check.h"  // def_dir_list_ok

int sys_getdents(int fd, uint8_t *buf, uint32_t count) {
    process_t *self = scheduler_get_current_process();
    if (!self || !buf || fd < 0 || fd >= MAX_FDS || !self->fd_table[fd])
        return -1;                                  // EBADF
    vfs_node_t *dir = self->fd_table[fd];
    // DEFENCE dir_read_check: read permission now, not only at open().
    if (!def_dir_list_ok(dir))
        return -1;

    uint32_t used = 0, index = self->fd_offset[fd];
    for (;; index++) {
        const char *name;
        uint32_t ino;
        uint8_t type = DT_DIR;
        if (index == 0)      { name = ".";  ino = dir->ino; }
        else if (index == 1) { name = ".."; ino = dir->parent ? dir->parent->ino : dir->ino; }
        else {
            vfs_node_t *child = vfs_readdir(dir, index - 2);
            if (!child)
                break;                              // the end
            name = child->name;
            ino  = child->ino;
            type = child->flags & FS_DIRECTORY ? DT_DIR : child->flags & FS_LINK ? DT_LNK : DT_REG;
        }
        uint32_t len = strlen(name);
        uint32_t reclen = (10 + len + 1 + 1 + 3) & ~3u;
        if (used + reclen > count) {
            if (!used)
                return -1;                          // EINVAL: not even one fits
            break;
        }
        uint8_t *rec = buf + used;
        uint32_t next = index + 1;
        *(uint32_t *)(rec + 0) = ino;
        *(uint32_t *)(rec + 4) = next;
        *(uint16_t *)(rec + 8) = (uint16_t)reclen;
        memcpy(rec + 10, name, len + 1);
        rec[reclen - 1] = type;
        used += reclen;
    }
    self->fd_offset[fd] = index;
    return (int)used;
}

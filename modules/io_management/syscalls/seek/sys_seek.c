/*
 * sys_seek.c - seek(fd, offset, whence): jump to another place in a file.
 *
 *   seek(fd, 0, SEEK_SET)    back to the start
 *   seek(fd, -4, SEEK_END)   the last 4 bytes
 *
 * The new position must be between 0 and the largest file size.
 *
 * Uses: the fd table (core/fd), RAMDISK_MAX_FILE_SIZE (filesystem_management),
 *       and the defence descriptor_bounds.
 */

#include "sys_seek.h"
#include "fd.h"
#include "ramdisk.h"
#include "scheduler.h"
#include "def_descriptor_bounds.h"

int sys_seek(int fd, int32_t offset, int whence) {
    process_t *self = scheduler_get_current_process();
    // DEFENCE descriptor_bounds: an open file (the screen has no position).
    if (!def_fd_ok(self, fd, 0))
        return -1;

    int64_t base;
    switch (whence) {
        case SEEK_SET: base = 0;                          break;
        case SEEK_CUR: base = self->fd_offset[fd];        break;
        case SEEK_END: base = self->fd_table[fd]->size;   break;
        default:       return -1;
    }

    int64_t position = base + offset;       // 64-bit: no overflow
    if (position < 0 || position > RAMDISK_MAX_FILE_SIZE)
        return -1;
    self->fd_offset[fd] = (uint32_t)position;
    return (int)position;
}

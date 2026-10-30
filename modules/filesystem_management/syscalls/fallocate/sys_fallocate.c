/*
 * sys_fallocate.c - fallocate(fd, mode, offset, length).
 *
 * Make sure bytes [offset, offset + length) exist, so later writes there
 * cannot fail for lack of space; the file grows if needed (new bytes 0).
 * Only mode 0 (Linux's other modes punch holes or keep the size). The
 * end must not wrap around and must pass size_cap. Returns 0 or -1.
 *
 * Uses: the current process (process_management), ramdisk_resize
 *       (core/ramdisk), O_WRITE (io_management), def_new_size_ok
 *       (security/huge_file/defence/size_cap).
 */
#include "modules/filesystem_management/syscalls/fallocate/sys_fallocate.h"   // this file
#include "modules/process_management/core/scheduler/scheduler.h"            // scheduler_get_current_process
#include "modules/filesystem_management/core/ramdisk/ramdisk.h"             // ramdisk_resize
#include "modules/io_management/syscalls/open/sys_open.h"                   // O_WRITE
#include "modules/filesystem_management/security/huge_file/defence/size_cap/def_size_cap.h"  // def_new_size_ok

int sys_fallocate(int fd, int mode, uint32_t offset, uint32_t length) {
    process_t *self = scheduler_get_current_process();
    if (!self || mode != 0 || length == 0 || fd < 0 || fd >= MAX_FDS ||
        !self->fd_table[fd] || !(self->fd_flags[fd] & O_WRITE))
        return -1;
    uint32_t end = offset + length;
    if (end < offset)
        return -1;                                  // wraps around: EFBIG
    if (end <= self->fd_table[fd]->size)
        return 0;                                   // already there
    if (!def_new_size_ok(end))                      // DEFENCE size_cap
        return -1;
    return ramdisk_resize(self->fd_table[fd], end);
}

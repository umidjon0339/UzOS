/*
 * sys_ftruncate.c - ftruncate(fd, length): truncate() for an open file.
 *
 * The descriptor must be open for writing (O_WRITE): the right was
 * checked when it was opened. size_cap as for truncate. Returns 0 or -1.
 *
 * Uses: the current process (process_management), ramdisk_resize
 *       (core/ramdisk), O_WRITE (io_management), def_new_size_ok
 *       (security/huge_file/defence/size_cap).
 */
#include "modules/filesystem_management/syscalls/ftruncate/sys_ftruncate.h"   // this file
#include "modules/process_management/core/scheduler/scheduler.h"            // scheduler_get_current_process
#include "modules/filesystem_management/core/ramdisk/ramdisk.h"             // ramdisk_resize
#include "modules/io_management/syscalls/open/sys_open.h"                   // O_WRITE
#include "modules/filesystem_management/security/huge_file/defence/size_cap/def_size_cap.h"  // def_new_size_ok

int sys_ftruncate(int fd, uint32_t length) {
    process_t *self = scheduler_get_current_process();
    if (!self || fd < 0 || fd >= MAX_FDS || !self->fd_table[fd] || !(self->fd_flags[fd] & O_WRITE))
        return -1;                                  // EBADF / EINVAL
    if (!def_new_size_ok(length))                   // DEFENCE size_cap
        return -1;
    return ramdisk_resize(self->fd_table[fd], length);
}

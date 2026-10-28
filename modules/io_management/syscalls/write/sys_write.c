/*
 * sys_write.c - write(fd, buf, count).
 *
 *   fd 1, 2 (stdout, stderr)  print on the screen
 *   fd 3+ (files)             write into the file at the descriptor's
 *                             offset (or at the end with O_APPEND)
 *
 * Uses: fd table of the current process (core/fd), vfs_write
 *       (filesystem_management), vga_put_char (device_management),
 *       and the defences descriptor_bounds and transfer_bounds.
 */

#include "sys_write.h"
#include "sys_open.h"
#include "fd.h"
#include "vga.h"
#include "scheduler.h"
#include "def_descriptor_bounds.h"
#include "def_transfer_bounds.h"
#include "modules/cpu_management/core/rlimit/rlimit.h"     // rlimit_fsize_ok

int sys_write(int fd, const void *buf, uint32_t count) {
    process_t *self = scheduler_get_current_process();

    // DEFENCE descriptor_bounds: a real descriptor, opened for writing.
    if (!def_fd_ok(self, fd, O_WRITE))
        return -1;
    // DEFENCE transfer_bounds: a real buffer, not too big.
    if (!def_transfer_ok(buf, count))
        return -1;

    if (fd == STDOUT || fd == STDERR) {
        const char *text = buf;
        for (uint32_t i = 0; i < count; i++)
            vga_put_char(text[i]);
        return count;
    }

    vfs_node_t *file = self->fd_table[fd];
    if (self->fd_flags[fd] & O_APPEND)
        self->fd_offset[fd] = file->size;   // always add at the end

    // RLIMIT_FSIZE (cpu_management/core/rlimit): no file past the limit.
    if (!rlimit_fsize_ok(self, self->fd_offset[fd] + count))
        return -1;

    uint32_t n = vfs_write(file, self->fd_offset[fd], count, (uint8_t *)buf);
    self->fd_offset[fd] += n;
    return (count && !n) ? -1 : (int)n;     // nothing written = file full
}

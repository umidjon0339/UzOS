/*
 * sys_read.c - read(fd, buf, count).
 *
 *   fd 0 (stdin)  the keys typed so far (does not wait for more)
 *   fd 3+ (files) bytes from the file, starting at the descriptor's
 *                 offset, which then moves forward
 *
 * Uses: fd table of the current process (core/fd), vfs_read
 *       (filesystem_management), keyboard_pop_key (device_management),
 *       and the defences descriptor_bounds and transfer_bounds.
 */

#include "sys_read.h"
#include "sys_open.h"
#include "fd.h"
#include "keyboard.h"
#include "scheduler.h"
#include "def_descriptor_bounds.h"
#include "def_transfer_bounds.h"

static int read_keyboard(char *out, uint32_t count) {
    uint32_t n = 0;
    char key;
    while (n < count && (key = keyboard_pop_key()))
        out[n++] = key;
    return n;
}

int sys_read(int fd, void *buf, uint32_t count) {
    process_t *self = scheduler_get_current_process();

    // DEFENCE descriptor_bounds: a real descriptor, opened for reading.
    if (!def_fd_ok(self, fd, O_READ))
        return -1;
    // DEFENCE transfer_bounds: a real buffer, not too big.
    if (!def_transfer_ok(buf, count))
        return -1;

    if (fd == STDIN)
        return read_keyboard(buf, count);

    uint32_t n = vfs_read(self->fd_table[fd], self->fd_offset[fd], count, buf);
    self->fd_offset[fd] += n;
    return n;
}

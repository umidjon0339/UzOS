/*
 * sys_close.c - close(fd): the number becomes free for the next open().
 *
 * Closing lowers the file's open count; once nobody has it open, it may
 * be deleted. Files a process forgets to close are closed for it when it
 * ends (fd_cleanup).
 *
 * Uses: fd_release (core/fd), and the defence descriptor_bounds.
 */

#include "sys_close.h"
#include "fd.h"
#include "scheduler.h"
#include "def_descriptor_bounds.h"

int sys_close(int fd) {
    process_t *self = scheduler_get_current_process();

    // DEFENCE descriptor_bounds: only an open file descriptor. Asking for
    // neither read nor write (0) also refuses stdin/stdout/stderr.
    if (!def_fd_ok(self, fd, 0))
        return -1;

    fd_release(self, fd);
    return 0;
}

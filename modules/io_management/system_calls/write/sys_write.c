#include "io_syscalls.h"
#include "io_security.h"
#include "scheduler.h"
#include "vfs.h"
#include "vga.h"
#include "proc_security.h"

int sys_write(int fd, const void *buf, uint32_t count) {
    process_t *current = scheduler_get_current_process();
    if (!current || !io_fd_index_valid(fd)) {
        sec_log("descriptor_bounds", "write used an invalid descriptor");
        return -1;
    }
    if (!count) return 0;
    if (!io_buffer_valid(buf, count)) {
        sec_log("transfer_bounds", "write buffer or size is invalid");
        return -1;
    }

    if (fd == IO_STDOUT || fd == IO_STDERR) {
        /* Both standard output streams currently share the VGA terminal. */
        const char *text = (const char *)buf;
        for (uint32_t i = 0; i < count; i++) vga_put_char(text[i]);
        return (int)count;
    }

    if (!io_file_descriptor_valid(current, fd)) {
        sec_log("descriptor_bounds", "write used a closed or non-writable descriptor");
        return -1;
    }

    vfs_node_t *node = current->fd_table[fd];
    uint32_t bytes_written = vfs_write(node, current->fd_offset[fd],
                                       count, (uint8_t *)buf);
    current->fd_offset[fd] += bytes_written;
    return (int)bytes_written;
}

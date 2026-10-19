#include "io_syscalls.h"
#include "io_security.h"
#include "scheduler.h"
#include "keyboard.h"
#include "vfs.h"
#include "proc_security.h"

int sys_read(int fd, void *buf, uint32_t count) {
    process_t *current = scheduler_get_current_process();
    if (!current || !io_fd_index_valid(fd)) {
        sec_log("descriptor_bounds", "read used an invalid descriptor");
        return -1;
    }
    if (!count) return 0;
    if (!io_buffer_valid(buf, count)) {
        sec_log("transfer_bounds", "read buffer or size is invalid");
        return -1;
    }

    if (fd == IO_STDIN) {
        /* Standard input is non-blocking and reads queued keyboard keys. */
        char *out = (char *)buf;
        uint32_t read = 0;
        while (read < count) {
            char key = keyboard_pop_key();
            if (!key) break;
            out[read++] = key;
        }
        return (int)read;
    }

    if (!io_file_descriptor_valid(current, fd)) {
        sec_log("descriptor_bounds", "read used a closed or non-readable descriptor");
        return -1;
    }

    vfs_node_t *node = current->fd_table[fd];
    uint32_t bytes_read = vfs_read(node, current->fd_offset[fd],
                                   count, (uint8_t *)buf);
    current->fd_offset[fd] += bytes_read;
    return (int)bytes_read;
}

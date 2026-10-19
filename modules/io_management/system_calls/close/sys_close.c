#include "io_syscalls.h"
#include "io_security.h"
#include "scheduler.h"
#include "vfs.h"
#include "proc_security.h"

int sys_close(int fd) {
    process_t *current = scheduler_get_current_process();
    if (!current || !io_fd_index_valid(fd) ||
        !io_file_descriptor_valid(current, fd)) {
        sec_log("descriptor_bounds", "close used a standard, invalid, or closed descriptor");
        return -1;
    }

    vfs_close(current->fd_table[fd]);
    current->fd_table[fd] = 0;
    current->fd_offset[fd] = 0;
    return 0;
}

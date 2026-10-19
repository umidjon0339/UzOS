#include "io_security.h"
#include "io_internal.h"
#include "mem_security.h"

int io_fd_index_valid(int fd) {
    return fd >= 0 && fd < MAX_FDS;
}

int io_file_descriptor_valid(const process_t *proc, int fd) {
    return proc && io_fd_index_valid(fd) &&
           fd >= IO_FIRST_FILE_FD && proc->fd_table[fd] != 0;
}

int io_buffer_valid(const void *buffer, uint32_t count) {
    return count <= IO_MAX_TRANSFER &&
           (!count || mem_validate_user_ptr(buffer, count));
}

int io_find_free_file_descriptor(const process_t *proc) {
    if (!proc) return -1;

    for (int fd = IO_FIRST_FILE_FD; fd < MAX_FDS; fd++)
        if (!proc->fd_table[fd]) return fd;
    return -1;
}

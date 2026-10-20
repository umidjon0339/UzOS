/*
 * fd.c - Hand out and take back descriptor numbers.
 *
 * Uses: vfs_open / vfs_close (filesystem_management) to keep each file's
 *       open count right, so busy_check knows when a file is in use.
 */

#include "fd.h"

int fd_alloc(process_t *p, vfs_node_t *node, uint32_t flags) {
    for (int fd = FIRST_FILE_FD; fd < MAX_FDS; fd++) {
        if (p->fd_table[fd])
            continue;
        p->fd_table[fd]  = node;
        p->fd_offset[fd] = 0;
        p->fd_flags[fd]  = flags;
        vfs_open(node);
        return fd;
    }
    return -1;
}

void fd_release(process_t *p, int fd) {
    vfs_close(p->fd_table[fd]);
    p->fd_table[fd]  = 0;
    p->fd_offset[fd] = 0;
    p->fd_flags[fd]  = 0;
}

int fd_count(const process_t *p) {
    int n = 0;
    for (int fd = FIRST_FILE_FD; fd < MAX_FDS; fd++)
        n += p->fd_table[fd] != 0;
    return n;
}

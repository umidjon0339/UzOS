/*
 * sys_socket.c - socket(domain, type, protocol): a new socket, as a file descriptor.
 *
 * The socket is made by core/socket, then put in the caller's descriptor
 * table like an open file (sockfs). SOCK_CLOEXEC in the type closes it
 * on execve, as O_CLOEXEC does for files.
 *
 * Uses: socket_create / socket_install_fd / socket_close (core/socket),
 *       scheduler_get_current_process (process_management), O_CLOEXEC.
 */
#include "modules/network_management/syscalls/socket/sys_socket.h"          // this file
#include "modules/network_management/core/socket/socket.h"                  // socket_*, sockaddr_t
#include "modules/network_management/core/inet/inet.h"                      // NET_E*
#include "modules/process_management/core/scheduler/scheduler.h"            // scheduler_get_current_process
#include "modules/io_management/syscalls/open/sys_open.h"                   // O_CLOEXEC

int sys_socket(int domain, int type, int protocol) {
    socket_t *s;
    int r = socket_create(domain, type, protocol, &s);
    if (r < 0)
        return r;
    process_t *self = scheduler_get_current_process();
    int fd = socket_install_fd(self, s);
    if (fd < 0) {
        socket_close(s);
        return fd;
    }
    if (type & SOCK_CLOEXEC)
        self->fd_flags[fd] |= O_CLOEXEC;
    return fd;
}

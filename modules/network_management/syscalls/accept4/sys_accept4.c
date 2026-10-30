/*
 * sys_accept4.c - accept4(fd, addr, len, flags): take the next waiting connection.
 *
 * A listening socket never carries data itself: each connection that
 * arrives becomes a NEW socket, and accept hands it out. 'addr' gets
 * who connected.
 *
 * Uses: socket_from_fd / socket_accept / socket_install_fd / socket_getname
 *       (core/socket), scheduler, O_CLOEXEC.
 */
#include "modules/network_management/syscalls/accept4/sys_accept4.h"        // this file
#include "modules/network_management/core/socket/socket.h"                  // socket_*, sockaddr_t
#include "modules/network_management/core/inet/inet.h"                      // NET_E*
#include "modules/process_management/core/scheduler/scheduler.h"            // scheduler_get_current_process
#include "modules/io_management/syscalls/open/sys_open.h"                   // O_CLOEXEC

int sys_accept4(int fd, sockaddr_t *addr, uint32_t *len, int flags) {
    process_t *self = scheduler_get_current_process();
    socket_t *s = socket_from_fd(self, fd);
    if (!s)
        return socket_fd_error(self, fd);   // -EBADF / -ENOTSOCK
    if (flags & ~(SOCK_NONBLOCK | SOCK_CLOEXEC))
        return -NET_EINVAL;
    socket_t *child;
    int r = socket_accept(s, &child);
    if (r < 0)
        return r;
    int new_fd = socket_install_fd(self, child);
    if (new_fd < 0) {
        socket_close(child);
        return new_fd;
    }
    child->nonblock = (flags & SOCK_NONBLOCK) != 0;
    if (flags & SOCK_CLOEXEC)
        self->fd_flags[new_fd] |= O_CLOEXEC;
    if (addr && len)
        socket_getname(child, 1, addr, len);
    return new_fd;
}

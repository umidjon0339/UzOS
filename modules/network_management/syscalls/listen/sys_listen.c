/*
 * sys_listen.c - listen(fd, backlog): wait for connections (a server).
 *
 * Uses: socket_from_fd / socket_listen (core/socket), scheduler.
 */
#include "modules/network_management/syscalls/listen/sys_listen.h"          // this file
#include "modules/network_management/core/socket/socket.h"                  // socket_*, sockaddr_t
#include "modules/network_management/core/inet/inet.h"                      // NET_E*
#include "modules/process_management/core/scheduler/scheduler.h"            // scheduler_get_current_process

int sys_listen(int fd, int backlog) {
    process_t *self = scheduler_get_current_process();
    socket_t *s = socket_from_fd(self, fd);
    if (!s)
        return socket_fd_error(self, fd);   // -EBADF / -ENOTSOCK
    return socket_listen(s, backlog);
}

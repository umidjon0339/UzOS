/*
 * sys_bind.c - bind(fd, addr, len): give a socket its own address and port.
 *
 * Port 0 asks for any free port. The address must be one of ours (or
 * 0.0.0.0: all of them).
 *
 * Uses: socket_from_fd / socket_bind (core/socket), scheduler.
 */
#include "modules/network_management/syscalls/bind/sys_bind.h"              // this file
#include "modules/network_management/core/socket/socket.h"                  // socket_*, sockaddr_t
#include "modules/network_management/core/inet/inet.h"                      // NET_E*
#include "modules/process_management/core/scheduler/scheduler.h"            // scheduler_get_current_process

int sys_bind(int fd, const sockaddr_t *addr, uint32_t len) {
    process_t *self = scheduler_get_current_process();
    socket_t *s = socket_from_fd(self, fd);
    if (!s)
        return socket_fd_error(self, fd);   // -EBADF / -ENOTSOCK
    return socket_bind(s, addr, len);
}

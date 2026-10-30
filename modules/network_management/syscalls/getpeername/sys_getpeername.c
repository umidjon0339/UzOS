/*
 * sys_getpeername.c - getpeername(fd, addr, len): the address of the other end.
 *
 * Uses: socket_from_fd / socket_getname (core/socket), scheduler.
 */
#include "modules/network_management/syscalls/getpeername/sys_getpeername.h"// this file
#include "modules/network_management/core/socket/socket.h"                  // socket_*, sockaddr_t
#include "modules/network_management/core/inet/inet.h"                      // NET_E*
#include "modules/process_management/core/scheduler/scheduler.h"            // scheduler_get_current_process

int sys_getpeername(int fd, sockaddr_t *addr, uint32_t *len) {
    process_t *self = scheduler_get_current_process();
    socket_t *s = socket_from_fd(self, fd);
    if (!s)
        return socket_fd_error(self, fd);   // -EBADF / -ENOTSOCK
    return socket_getname(s, 1, addr, len);
}

/*
 * sys_connect.c - connect(fd, addr, len): talk to one peer (TCP: make the connection).
 *
 * UDP: only remembers the peer. TCP: sends SYN and sleeps until the
 * SYN-ACK (or a refusal) comes back.
 *
 * Uses: socket_from_fd / socket_connect (core/socket), scheduler.
 */
#include "modules/network_management/syscalls/connect/sys_connect.h"        // this file
#include "modules/network_management/core/socket/socket.h"                  // socket_*, sockaddr_t
#include "modules/network_management/core/inet/inet.h"                      // NET_E*
#include "modules/process_management/core/scheduler/scheduler.h"            // scheduler_get_current_process

int sys_connect(int fd, const sockaddr_t *addr, uint32_t len) {
    process_t *self = scheduler_get_current_process();
    socket_t *s = socket_from_fd(self, fd);
    if (!s)
        return socket_fd_error(self, fd);   // -EBADF / -ENOTSOCK
    return socket_connect(s, addr, len);
}

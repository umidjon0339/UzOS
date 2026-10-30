/*
 * sys_sendto.c - sendto(fd, buf, len, flags, addr, addrlen): send one message.
 *
 * With addr 0 it is send(): the connected peer. Six arguments: the
 * sixth travels in EBP, as on 32-bit Linux.
 *
 * Uses: socket_from_fd / socket_sendto (core/socket), scheduler.
 */
#include "modules/network_management/syscalls/sendto/sys_sendto.h"          // this file
#include "modules/network_management/core/socket/socket.h"                  // socket_*, sockaddr_t
#include "modules/network_management/core/inet/inet.h"                      // NET_E*
#include "modules/process_management/core/scheduler/scheduler.h"            // scheduler_get_current_process

int sys_sendto(int fd, const void *buf, uint32_t len, int flags,
               const sockaddr_t *addr, uint32_t addrlen) {
    process_t *self = scheduler_get_current_process();
    socket_t *s = socket_from_fd(self, fd);
    if (!s)
        return socket_fd_error(self, fd);   // -EBADF / -ENOTSOCK
    return socket_sendto(s, buf, len, flags, addr, addrlen);
}

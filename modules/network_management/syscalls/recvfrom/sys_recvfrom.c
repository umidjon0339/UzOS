/*
 * sys_recvfrom.c - recvfrom(fd, buf, len, flags, addr, addrlen): receive one message.
 *
 * Sleeps until something arrives, unless the socket is non-blocking.
 * 'addr' gets who sent it.
 *
 * Uses: socket_from_fd / socket_recvfrom (core/socket), scheduler.
 */
#include "modules/network_management/syscalls/recvfrom/sys_recvfrom.h"      // this file
#include "modules/network_management/core/socket/socket.h"                  // socket_*, sockaddr_t
#include "modules/network_management/core/inet/inet.h"                      // NET_E*
#include "modules/process_management/core/scheduler/scheduler.h"            // scheduler_get_current_process

int sys_recvfrom(int fd, void *buf, uint32_t len, int flags,
                 sockaddr_t *addr, uint32_t *addrlen) {
    process_t *self = scheduler_get_current_process();
    socket_t *s = socket_from_fd(self, fd);
    if (!s)
        return socket_fd_error(self, fd);   // -EBADF / -ENOTSOCK
    return socket_recvfrom(s, buf, len, flags, addr, addrlen);
}

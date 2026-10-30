/*
 * sys_getsockopt.c - getsockopt(fd, level, name, value, len): read a socket option.
 *
 * Uses: socket_from_fd / socket_getopt (core/socket), scheduler.
 */
#include "modules/network_management/syscalls/getsockopt/sys_getsockopt.h"  // this file
#include "modules/network_management/core/socket/socket.h"                  // socket_*, sockaddr_t
#include "modules/network_management/core/inet/inet.h"                      // NET_E*
#include "modules/process_management/core/scheduler/scheduler.h"            // scheduler_get_current_process

int sys_getsockopt(int fd, int level, int name, void *value, uint32_t *len) {
    process_t *self = scheduler_get_current_process();
    socket_t *s = socket_from_fd(self, fd);
    if (!s)
        return socket_fd_error(self, fd);   // -EBADF / -ENOTSOCK
    return socket_getopt(s, level, name, value, len);
}

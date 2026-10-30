/*
 * sys_setsockopt.c - setsockopt(fd, level, name, value, len): change a socket option.
 *
 * The value is copied only as far as 'len' says, and must be at least
 * as long as the option needs (security/sockopt_overflow).
 *
 * Uses: socket_from_fd / socket_setopt (core/socket), scheduler.
 */
#include "modules/network_management/syscalls/setsockopt/sys_setsockopt.h"  // this file
#include "modules/network_management/core/socket/socket.h"                  // socket_*, sockaddr_t
#include "modules/network_management/core/inet/inet.h"                      // NET_E*
#include "modules/process_management/core/scheduler/scheduler.h"            // scheduler_get_current_process

int sys_setsockopt(int fd, int level, int name, const void *value, uint32_t len) {
    process_t *self = scheduler_get_current_process();
    socket_t *s = socket_from_fd(self, fd);
    if (!s)
        return socket_fd_error(self, fd);   // -EBADF / -ENOTSOCK
    return socket_setopt(s, level, name, value, len);
}

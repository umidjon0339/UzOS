/*
 * sys_accept.c - accept(fd, addr, len): accept4() with no flags.
 *
 * Uses: sys_accept4 (syscalls/accept4).
 */
#include "modules/network_management/syscalls/accept/sys_accept.h"          // this file
#include "modules/network_management/core/socket/socket.h"                  // socket_*, sockaddr_t
#include "modules/network_management/core/inet/inet.h"                      // NET_E*
#include "modules/network_management/syscalls/accept4/sys_accept4.h"        // sys_accept4

int sys_accept(int fd, sockaddr_t *addr, uint32_t *len) {
    return sys_accept4(fd, addr, len, 0);
}

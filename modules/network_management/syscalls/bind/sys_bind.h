#ifndef SYS_BIND_H
#define SYS_BIND_H
/* sys_bind.h - bind(fd, addr, len): give a socket its own address and port. */
#include <stdint.h>
#include "modules/network_management/core/socket/socket.h"                  // sockaddr_t, SOCK_*, AF_*
#define SYS_BIND 134   // EAX=134, EBX=fd, ECX=addr (sockaddr_in), EDX=len
// 0, or -errno: -EBADF/-ENOTSOCK, -EINVAL, -EADDRINUSE, -EADDRNOTAVAIL, -EACCES.
int sys_bind(int fd, const sockaddr_t *addr, uint32_t len);
#endif

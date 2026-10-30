#ifndef SYS_GETPEERNAME_H
#define SYS_GETPEERNAME_H
/* sys_getpeername.h - getpeername(fd, addr, len): the address of the other end. */
#include <stdint.h>
#include "modules/network_management/core/socket/socket.h"                  // sockaddr_t, SOCK_*, AF_*
#define SYS_GETPEERNAME 140   // EAX=140, EBX=fd, ECX=addr, EDX=&len
// 0, or -errno: -ENOTCONN if it is not connected.
int sys_getpeername(int fd, sockaddr_t *addr, uint32_t *len);
#endif

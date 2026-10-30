#ifndef SYS_CONNECT_H
#define SYS_CONNECT_H
/* sys_connect.h - connect(fd, addr, len): talk to one peer (TCP: make the connection). */
#include <stdint.h>
#include "modules/network_management/core/socket/socket.h"                  // sockaddr_t, SOCK_*, AF_*
#define SYS_CONNECT 135   // EAX=135, EBX=fd, ECX=addr, EDX=len
// 0, or -errno: -ECONNREFUSED, -ETIMEDOUT, -ENETUNREACH, -EINPROGRESS
// (non-blocking: still connecting), -EINTR (a signal).
int sys_connect(int fd, const sockaddr_t *addr, uint32_t len);
#endif

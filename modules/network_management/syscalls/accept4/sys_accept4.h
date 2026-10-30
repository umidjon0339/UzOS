#ifndef SYS_ACCEPT4_H
#define SYS_ACCEPT4_H
/* sys_accept4.h - accept4(fd, addr, len, flags): take the next waiting connection. */
#include <stdint.h>
#include "modules/network_management/core/socket/socket.h"                  // sockaddr_t, SOCK_*, AF_*
#define SYS_ACCEPT4 138   // EAX=138, EBX=fd, ECX=addr (or 0), EDX=&len, ESI=flags (SOCK_NONBLOCK | SOCK_CLOEXEC)
// The new connection's descriptor, or -errno: -EAGAIN (non-blocking,
// none waiting), -EINVAL (not listening), -EMFILE, -EINTR.
int sys_accept4(int fd, sockaddr_t *addr, uint32_t *len, int flags);
#endif

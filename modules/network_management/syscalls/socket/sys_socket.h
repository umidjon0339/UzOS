#ifndef SYS_SOCKET_H
#define SYS_SOCKET_H
/* sys_socket.h - socket(domain, type, protocol): a new socket, as a file descriptor. */
#include <stdint.h>
#include "modules/network_management/core/socket/socket.h"                  // sockaddr_t, SOCK_*, AF_*
#define SYS_SOCKET 133   // EAX=133, EBX=domain (AF_INET), ECX=type (SOCK_DGRAM | SOCK_NONBLOCK ...), EDX=protocol (0)
// The descriptor, or -errno: -EAFNOSUPPORT, -EPROTONOSUPPORT,
// -EPERM (SOCK_RAW without CAP_NET_RAW), -EMFILE, -ENOBUFS.
int sys_socket(int domain, int type, int protocol);
#endif

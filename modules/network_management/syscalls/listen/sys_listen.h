#ifndef SYS_LISTEN_H
#define SYS_LISTEN_H
/* sys_listen.h - listen(fd, backlog): wait for connections (a server). */
#include <stdint.h>
#include "modules/network_management/core/socket/socket.h"                  // sockaddr_t, SOCK_*, AF_*
#define SYS_LISTEN 136   // EAX=136, EBX=fd, ECX=backlog (connections that may wait for accept)
// 0, or -errno: -EOPNOTSUPP (not a stream socket), -EINVAL.
int sys_listen(int fd, int backlog);
#endif

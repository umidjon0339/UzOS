#ifndef SYS_GETSOCKOPT_H
#define SYS_GETSOCKOPT_H
/* sys_getsockopt.h - getsockopt(fd, level, name, value, len): read a socket option. */
#include <stdint.h>
#include "modules/network_management/core/socket/socket.h"                  // sockaddr_t, SOCK_*, AF_*
#define SYS_GETSOCKOPT 148   // EAX=148, EBX=fd, ECX=level, EDX=name, ESI=value, EDI=&len (in: room, out: size)
// 0, or -errno. SO_ERROR returns (and clears) a pending error.
int sys_getsockopt(int fd, int level, int name, void *value, uint32_t *len);
#endif

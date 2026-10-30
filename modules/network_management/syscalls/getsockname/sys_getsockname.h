#ifndef SYS_GETSOCKNAME_H
#define SYS_GETSOCKNAME_H
/* sys_getsockname.h - getsockname(fd, addr, len): this socket's own address and port. */
#include <stdint.h>
#include "modules/network_management/core/socket/socket.h"                  // sockaddr_t, SOCK_*, AF_*
#define SYS_GETSOCKNAME 139   // EAX=139, EBX=fd, ECX=addr, EDX=&len (in: room, out: size)
// 0, or -errno. After bind(port 0) it tells which port was chosen.
int sys_getsockname(int fd, sockaddr_t *addr, uint32_t *len);
#endif

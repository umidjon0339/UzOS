#ifndef SYS_ACCEPT_H
#define SYS_ACCEPT_H
/* sys_accept.h - accept(fd, addr, len): accept4() with no flags. */
#include <stdint.h>
#include "modules/network_management/core/socket/socket.h"                  // sockaddr_t, SOCK_*, AF_*
#define SYS_ACCEPT 137   // EAX=137, EBX=fd, ECX=addr (or 0), EDX=&len
// The new connection's descriptor, or -errno (see accept4).
int sys_accept(int fd, sockaddr_t *addr, uint32_t *len);
#endif

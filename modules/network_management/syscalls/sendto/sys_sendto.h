#ifndef SYS_SENDTO_H
#define SYS_SENDTO_H
/* sys_sendto.h - sendto(fd, buf, len, flags, addr, addrlen): send one message. */
#include <stdint.h>
#include "modules/network_management/core/socket/socket.h"                  // sockaddr_t, SOCK_*, AF_*
#define SYS_SENDTO 142   // EAX=142, EBX=fd, ECX=buf, EDX=len, ESI=flags, EDI=addr (or 0), EBP=addrlen
// Bytes sent, or -errno: -EDESTADDRREQ (no address, not connected),
// -EMSGSIZE, -EACCES (broadcast without SO_BROADCAST), -EPIPE, -EAGAIN.
int sys_sendto(int fd, const void *buf, uint32_t len, int flags,
               const sockaddr_t *addr, uint32_t addrlen);
#endif

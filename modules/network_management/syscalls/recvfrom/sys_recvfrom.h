#ifndef SYS_RECVFROM_H
#define SYS_RECVFROM_H
/* sys_recvfrom.h - recvfrom(fd, buf, len, flags, addr, addrlen): receive one message. */
#include <stdint.h>
#include "modules/network_management/core/socket/socket.h"                  // sockaddr_t, SOCK_*, AF_*
#define SYS_RECVFROM 143   // EAX=143, EBX=fd, ECX=buf, EDX=len, ESI=flags (MSG_PEEK, MSG_DONTWAIT), EDI=addr (or 0), EBP=&addrlen
// Bytes received (0 = the peer closed), or -errno: -EAGAIN (nothing,
// non-blocking or SO_RCVTIMEO), -EINTR, -ECONNREFUSED.
int sys_recvfrom(int fd, void *buf, uint32_t len, int flags,
                 sockaddr_t *addr, uint32_t *addrlen);
#endif

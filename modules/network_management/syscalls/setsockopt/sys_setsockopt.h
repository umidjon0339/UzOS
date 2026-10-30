#ifndef SYS_SETSOCKOPT_H
#define SYS_SETSOCKOPT_H
/* sys_setsockopt.h - setsockopt(fd, level, name, value, len): change a socket option. */
#include <stdint.h>
#include "modules/network_management/core/socket/socket.h"                  // sockaddr_t, SOCK_*, AF_*
#define SYS_SETSOCKOPT 147   // EAX=147, EBX=fd, ECX=level (SOL_SOCKET...), EDX=name (SO_REUSEADDR...), ESI=value, EDI=len
// 0, or -errno: -ENOPROTOOPT (unknown option), -EINVAL (len too short).
int sys_setsockopt(int fd, int level, int name, const void *value, uint32_t len);
#endif

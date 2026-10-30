#ifndef SYS_SHUTDOWN_H
#define SYS_SHUTDOWN_H
/* sys_shutdown.h - shutdown(fd, how): stop reading, writing, or both, without closing. */
#include <stdint.h>
#include "modules/network_management/core/socket/socket.h"                  // sockaddr_t, SOCK_*, AF_*
#define SYS_SHUTDOWN 146   // EAX=146, EBX=fd, ECX=how (SHUT_RD 0, SHUT_WR 1, SHUT_RDWR 2)
// 0, or -errno. TCP SHUT_WR sends FIN: "I have nothing more to say".
int sys_shutdown(int fd, int how);
#endif

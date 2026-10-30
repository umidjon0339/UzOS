#ifndef SYS_SOCKETPAIR_H
#define SYS_SOCKETPAIR_H
/* sys_socketpair.h - socketpair(domain, type, protocol, sv): two sockets joined to each other. */
#include <stdint.h>
#define SYS_SOCKETPAIR 141  // EAX=141, EBX=domain (AF_UNIX), ECX=type, EDX=protocol (0), ESI=sv (int[2])
// 0 and sv[0], sv[1] filled, or -errno: -EAFNOSUPPORT (only AF_UNIX), -EMFILE.
int sys_socketpair(int domain, int type, int protocol, int sv[2]);
#endif

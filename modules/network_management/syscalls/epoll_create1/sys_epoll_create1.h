#ifndef SYS_EPOLL_CREATE1_H
#define SYS_EPOLL_CREATE1_H
/* sys_epoll_create1.h - epoll_create1(flags): a new epoll instance, as a descriptor. */
#include <stdint.h>
#define SYS_EPOLL_CREATE1 157   // EAX=157, EBX=flags (0 or EPOLL_CLOEXEC)
// The descriptor, or -errno: -EINVAL (bad flags), -EMFILE, -ENOMEM (no instance free).
int sys_epoll_create1(int flags);
#endif

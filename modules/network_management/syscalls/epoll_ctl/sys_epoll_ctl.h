#ifndef SYS_EPOLL_CTL_H
#define SYS_EPOLL_CTL_H
/* sys_epoll_ctl.h - epoll_ctl(epfd, op, fd, event): add, change or remove a watched descriptor. */
#include <stdint.h>
#include "modules/network_management/core/epoll/epoll.h"       // epoll_event_t, EPOLL_CTL_*
#define SYS_EPOLL_CTL 158       // EAX=158, EBX=epfd, ECX=op (ADD 1, DEL 2, MOD 3), EDX=fd, ESI=event
// 0, or -errno: -EBADF, -EINVAL, -EEXIST (as -EADDRINUSE here), -ENOMEM.
int sys_epoll_ctl(int epfd, int op, int fd, const epoll_event_t *event);
#endif

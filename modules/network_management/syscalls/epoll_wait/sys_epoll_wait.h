#ifndef SYS_EPOLL_WAIT_H
#define SYS_EPOLL_WAIT_H
/* sys_epoll_wait.h - epoll_wait(epfd, events, max, timeout): which watched descriptors are ready. */
#include <stdint.h>
#include "modules/network_management/core/epoll/epoll.h"       // epoll_event_t
#define SYS_EPOLL_WAIT 159      // EAX=159, EBX=epfd, ECX=events (epoll_event_t[]), EDX=max, ESI=timeout ms
// How many events were written, 0 on timeout, or -errno (-EINTR, -EINVAL).
int sys_epoll_wait(int epfd, epoll_event_t *events, int max, int timeout_ms);
#endif

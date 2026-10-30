#ifndef SYS_EPOLL_PWAIT_H
#define SYS_EPOLL_PWAIT_H
/* sys_epoll_pwait.h - epoll_pwait(epfd, events, max, timeout, sigmask, size): epoll_wait with a signal mask. */
#include <stdint.h>
#include "modules/network_management/core/epoll/epoll.h"       // epoll_event_t
#define SYS_EPOLL_PWAIT 160     // EAX=160, EBX=epfd, ECX=events, EDX=max, ESI=timeout ms, EDI=sigmask, EBP=size
int sys_epoll_pwait(int epfd, epoll_event_t *events, int max, int timeout_ms,
                    const uint32_t *sigmask, uint32_t size);
#endif

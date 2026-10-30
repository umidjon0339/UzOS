/*
 * sys_epoll_pwait.c - epoll_pwait(): epoll_wait that swaps the signal
 * mask while it waits (see ppoll for why).
 *
 * Uses: epoll_wait_events (core/epoll), scheduler, SIG_UNBLOCKABLE.
 */
#include "modules/network_management/syscalls/epoll_pwait/sys_epoll_pwait.h" // this file
#include "modules/network_management/core/inet/inet.h"                      // NET_EINVAL
#include "modules/process_management/core/scheduler/scheduler.h"            // scheduler_get_current_process
#include "modules/process_management/core/signal/signal.h"                  // SIG_UNBLOCKABLE

int sys_epoll_pwait(int epfd, epoll_event_t *events, int max, int timeout_ms,
                    const uint32_t *sigmask, uint32_t size) {
    if (sigmask && size < sizeof(uint32_t))
        return -NET_EINVAL;
    process_t *self = scheduler_get_current_process();
    uint32_t old = self->sig_blocked;
    if (sigmask)
        self->sig_blocked = *sigmask & ~SIG_UNBLOCKABLE;
    int r = epoll_wait_events(self, epfd, events, max, timeout_ms);
    self->sig_blocked = old;
    return r;
}

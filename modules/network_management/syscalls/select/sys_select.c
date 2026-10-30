/*
 * sys_select.c - select(): turn the bitmaps into a poll list, wait, and
 * turn the answer back into bitmaps.
 *
 *   read set     -> POLLIN    (or a hang-up: reading returns 0)
 *   write set    -> POLLOUT
 *   except set   -> POLLPRI   (urgent data: never in UzOS)
 *
 * Uses: poll_wait (core/poll), scheduler.
 */
#include "modules/network_management/syscalls/select/sys_select.h"          // this file
#include "modules/network_management/core/poll/poll.h"                      // poll_wait, pollfd_t
#include "modules/network_management/core/socket/socket.h"                  // POLLIN, POLLOUT, ...
#include "modules/network_management/core/inet/inet.h"                      // NET_EINVAL
#include "modules/process_management/core/scheduler/scheduler.h"            // scheduler_get_current_process

int select_wait(int nfds, select_set_t *rd, select_set_t *wr, select_set_t *ex, int timeout_ms) {
    if (nfds < 0 || nfds > SELECT_FDS)
        return -NET_EINVAL;
    pollfd_t fds[SELECT_FDS];
    int count = 0;
    for (int fd = 0; fd < nfds; fd++) {
        uint32_t bit = 1u << fd;
        int16_t events = (int16_t)(((rd && (rd->bits & bit)) ? POLLIN : 0) |
                                   ((wr && (wr->bits & bit)) ? POLLOUT : 0) |
                                   ((ex && (ex->bits & bit)) ? POLLPRI : 0));
        if (events) {
            fds[count].fd = fd;
            fds[count].events = events;
            count++;
        }
    }
    int r = poll_wait(scheduler_get_current_process(), fds, count, timeout_ms);
    if (r < 0)
        return r;
    if (rd) rd->bits = 0;
    if (wr) wr->bits = 0;
    if (ex) ex->bits = 0;
    int bits = 0;
    for (int i = 0; i < count; i++) {
        uint32_t bit = 1u << fds[i].fd;
        int got = fds[i].revents;
        if (rd && (fds[i].events & POLLIN) && (got & (POLLIN | POLLHUP | POLLERR))) {
            rd->bits |= bit;
            bits++;
        }
        if (wr && (fds[i].events & POLLOUT) && (got & (POLLOUT | POLLERR))) {
            wr->bits |= bit;
            bits++;
        }
        if (ex && (fds[i].events & POLLPRI) && (got & POLLPRI)) {
            ex->bits |= bit;
            bits++;
        }
    }
    return bits;
}

int sys_select(int nfds, select_set_t *rd, select_set_t *wr, select_set_t *ex,
               const select_timeval_t *timeout) {
    int ms = timeout ? (int)(timeout->sec * 1000 + timeout->usec / 1000) : -1;
    return select_wait(nfds, rd, wr, ex, ms);
}

/*
 * poll.c - Readiness of a descriptor, and the waiting loop.
 *
 * Uses: socket_from_fd / socket_poll (core/socket), netdev_lock
 *       (core/netdev), keyboard_has_key (device_management),
 *       sys_sleep_interruptible (process_management), NET_EINTR (core/inet).
 */

#include "modules/network_management/core/poll/poll.h"           // this file
#include "modules/network_management/core/socket/socket.h"       // socket_from_fd, socket_poll, POLL*
#include "modules/network_management/core/netdev/netdev.h"       // netdev_lock
#include "modules/network_management/core/inet/inet.h"          // NET_EINTR
#include "modules/io_management/core/fd/fd.h"                    // STDIN, STDOUT, STDERR
#include "modules/device_management/keyboard/keyboard.h"         // keyboard_has_key
#include "modules/process_management/syscalls/sleep/sys_sleep.h" // sys_sleep_interruptible

int poll_ready(process_t *p, int fd) {
    if (fd == STDIN)
        return keyboard_has_key() ? POLLIN : 0;
    if (fd == STDOUT || fd == STDERR)
        return POLLOUT;
    if (fd < 0 || fd >= MAX_FDS || !p->fd_table[fd])
        return POLLNVAL;
    socket_t *s = socket_from_fd(p, fd);
    if (!s)
        return POLLIN | POLLOUT;                // a file: never makes you wait
    uint32_t lock = netdev_lock();
    int bits = socket_poll(s);
    netdev_unlock(lock);
    return bits;
}

// Ask every socket among fds to wake p (or forget p: who = NULL).
static void set_waiter(process_t *p, pollfd_t *fds, int count, process_t *who) {
    for (int i = 0; i < count; i++) {
        socket_t *s = socket_from_fd(p, fds[i].fd);
        if (s)
            s->waiter = who;
    }
}

int poll_wait(process_t *p, pollfd_t *fds, int count, int timeout_ms) {
    int waited = 0;
    for (;;) {
        int ready = 0;
        for (int i = 0; i < count; i++) {
            fds[i].revents = 0;
            if (fds[i].fd < 0)
                continue;                       // a negative fd is skipped (as Linux)
            // POLLERR, POLLHUP and POLLNVAL are reported even if not asked for.
            int bits = poll_ready(p, fds[i].fd);
            fds[i].revents = (int16_t)(bits & (fds[i].events | POLLERR | POLLHUP | POLLNVAL));
            ready += fds[i].revents != 0;
        }
        if (ready || timeout_ms == 0 || (timeout_ms > 0 && waited >= timeout_ms))
            return ready;

        set_waiter(p, fds, count, p);
        int calm = sys_sleep_interruptible(POLL_SLICE_MS);
        set_waiter(p, fds, count, 0);
        waited += POLL_SLICE_MS;
        if (!calm)
            return -NET_EINTR;
    }
}

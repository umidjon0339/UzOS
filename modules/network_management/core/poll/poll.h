#ifndef POLL_H
#define POLL_H

/*
 * poll.h - Wait for ANY of several descriptors to become ready.
 *
 * A server with many clients cannot sit in recv() on one of them while
 * the others talk. Instead it asks "tell me when one of these has
 * something": select(), poll() and epoll_wait() all end up here.
 *
 *   sockets          ready as socket_poll() says (data, room, a connection, hang-up)
 *   stdin (fd 0)     ready to read when a key is waiting
 *   stdout, stderr   always ready to write
 *   files            always ready (a RAM disk never makes you wait)
 *
 * While nothing is ready the process sleeps. Every socket it waits on
 * remembers it (socket.waiter), so the first packet that arrives wakes
 * it at once; otherwise it looks again every POLL_SLICE_MS.
 */

#include <stdint.h>
#include "modules/process_management/core/process/process.h"    // process_t

#define POLL_SLICE_MS 20

// Linux's struct pollfd.
typedef struct {
    int fd;
    int16_t events;             // what we wait for: POLLIN, POLLOUT, ...
    int16_t revents;            // what happened (filled in)
} pollfd_t;

// What fd of p is ready for right now: POLL* bits (POLLNVAL if not open).
int poll_ready(process_t *p, int fd);

// Fill each revents; wait up to timeout_ms (-1 = forever, 0 = only look).
// Returns how many are ready, 0 on timeout, or -NET_EINTR (a signal).
int poll_wait(process_t *p, pollfd_t *fds, int count, int timeout_ms);

#endif

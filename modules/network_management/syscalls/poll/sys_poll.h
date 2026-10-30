#ifndef SYS_POLL_H
#define SYS_POLL_H
/* sys_poll.h - poll(fds, n, timeout): wait until one of n descriptors is ready. */
#include <stdint.h>
#include "modules/network_management/core/poll/poll.h"         // pollfd_t
#define SYS_POLL   154      // EAX=154, EBX=fds (pollfd_t[]), ECX=n, EDX=timeout ms (-1 = forever)
#define POLL_MAX   32       // descriptors per call
// How many have revents set, 0 on timeout, or -errno (-EINTR, -EINVAL).
int sys_poll(pollfd_t *fds, uint32_t n, int timeout_ms);
#endif

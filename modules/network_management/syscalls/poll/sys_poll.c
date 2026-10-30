/*
 * sys_poll.c - poll(): the program lists the descriptors and what it
 * waits for (POLLIN, POLLOUT); the kernel fills in what happened.
 *
 * Uses: poll_wait (core/poll), scheduler.
 */
#include "modules/network_management/syscalls/poll/sys_poll.h"              // this file
#include "modules/network_management/core/inet/inet.h"                      // NET_EINVAL
#include "modules/process_management/core/scheduler/scheduler.h"            // scheduler_get_current_process

int sys_poll(pollfd_t *fds, uint32_t n, int timeout_ms) {
    if (n > POLL_MAX)
        return -NET_EINVAL;
    return poll_wait(scheduler_get_current_process(), fds, (int)n, timeout_ms);
}

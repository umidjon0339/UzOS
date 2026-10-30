#ifndef SYS_PPOLL_H
#define SYS_PPOLL_H
/* sys_ppoll.h - ppoll(fds, n, timeout, sigmask, size): poll() with a timespec and a signal mask. */
#include <stdint.h>
#include "modules/network_management/core/poll/poll.h"         // pollfd_t
#define SYS_PPOLL 155       // EAX=155, EBX=fds, ECX=n, EDX=timeout (timespec*, 0 = forever), ESI=sigmask (or 0), EDI=size

// struct timespec: seconds and nanoseconds.
typedef struct {
    uint32_t sec, nsec;
} ppoll_timespec_t;

// As poll(); while it waits, the signals in *sigmask are blocked instead.
int sys_ppoll(pollfd_t *fds, uint32_t n, const ppoll_timespec_t *timeout,
              const uint32_t *sigmask, uint32_t size);
#endif

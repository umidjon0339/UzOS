/*
 * sys_ppoll.c - ppoll(): poll() that swaps the signal mask while it waits.
 *
 * Why the mask: a program that blocks SIGINT, checks a flag, then
 * waits would miss a signal that comes between the check and the wait.
 * ppoll unblocks it and waits IN ONE STEP, then puts the mask back.
 * (Only the low 32 signals exist in UzOS.)
 *
 * Uses: poll_wait (core/poll), scheduler, SIG_UNBLOCKABLE (core/signal).
 */
#include "modules/network_management/syscalls/ppoll/sys_ppoll.h"            // this file
#include "modules/network_management/syscalls/poll/sys_poll.h"              // POLL_MAX
#include "modules/network_management/core/inet/inet.h"                      // NET_EINVAL
#include "modules/process_management/core/scheduler/scheduler.h"            // scheduler_get_current_process
#include "modules/process_management/core/signal/signal.h"                  // SIG_UNBLOCKABLE

int sys_ppoll(pollfd_t *fds, uint32_t n, const ppoll_timespec_t *timeout,
              const uint32_t *sigmask, uint32_t size) {
    if (n > POLL_MAX || (sigmask && size < sizeof(uint32_t)))
        return -NET_EINVAL;
    int ms = timeout ? (int)(timeout->sec * 1000 + timeout->nsec / 1000000) : -1;
    process_t *self = scheduler_get_current_process();
    uint32_t old = self->sig_blocked;
    if (sigmask)
        self->sig_blocked = *sigmask & ~SIG_UNBLOCKABLE;
    int r = poll_wait(self, fds, (int)n, ms);
    self->sig_blocked = old;
    return r;
}

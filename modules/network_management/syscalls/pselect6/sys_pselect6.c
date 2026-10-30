/*
 * sys_pselect6.c - pselect6(): select() that swaps the signal mask while
 * it waits (see ppoll for why).
 *
 * Uses: select_wait (syscalls/select), scheduler, SIG_UNBLOCKABLE.
 */
#include "modules/network_management/syscalls/pselect6/sys_pselect6.h"      // this file
#include "modules/network_management/core/inet/inet.h"                      // NET_EINVAL
#include "modules/process_management/core/scheduler/scheduler.h"            // scheduler_get_current_process
#include "modules/process_management/core/signal/signal.h"                  // SIG_UNBLOCKABLE

int sys_pselect6(int nfds, select_set_t *rd, select_set_t *wr, select_set_t *ex,
                 const pselect_timespec_t *timeout, const pselect_sigdata_t *sigdata) {
    int ms = timeout ? (int)(timeout->sec * 1000 + timeout->nsec / 1000000) : -1;
    const uint32_t *mask = sigdata ? sigdata->mask : 0;
    if (mask && sigdata->size < sizeof(uint32_t))
        return -NET_EINVAL;
    process_t *self = scheduler_get_current_process();
    uint32_t old = self->sig_blocked;
    if (mask)
        self->sig_blocked = *mask & ~SIG_UNBLOCKABLE;
    int r = select_wait(nfds, rd, wr, ex, ms);
    self->sig_blocked = old;
    return r;
}

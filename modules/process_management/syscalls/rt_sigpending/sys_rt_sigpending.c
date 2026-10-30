/*
 * sys_rt_sigpending.c - rt_sigpending(set, sigsetsize): the signals sent
 * to the caller and not delivered yet (in practice: the blocked ones).
 *
 * Uses: scheduler_get_current_process (core/scheduler).
 */
#include "modules/process_management/syscalls/rt_sigpending/sys_rt_sigpending.h"  // this file
#include "modules/process_management/core/scheduler/scheduler.h"                  // scheduler_get_current_process

int sys_rt_sigpending(uint32_t *set, uint32_t sigsetsize) {
    process_t *self = scheduler_get_current_process();
    if (!self || !set || (sigsetsize != 4 && sigsetsize != 8))
        return -1;
    set[0] = self->signals;
    if (sigsetsize == 8)
        set[1] = 0;
    return 0;
}

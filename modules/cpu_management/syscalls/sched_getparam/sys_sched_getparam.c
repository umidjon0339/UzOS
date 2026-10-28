/*
 * sys_sched_getparam.c - sched_getparam(): a process's real-time priority.
 *
 * 1..99 for SCHED_FIFO / SCHED_RR, 0 for every other policy.
 *
 * Uses: process_get_by_pid and the current process (process_management).
 */

#include "modules/cpu_management/syscalls/sched_getparam/sys_sched_getparam.h"
#include "modules/process_management/core/scheduler/scheduler.h"    // the current process

int sys_sched_getparam(uint32_t pid, sched_param_t *param) {
    process_t *p = pid ? process_get_by_pid(pid) : scheduler_get_current_process();
    if (!p || !param)
        return -1;
    param->sched_priority = p->rt_priority;
    return 0;
}

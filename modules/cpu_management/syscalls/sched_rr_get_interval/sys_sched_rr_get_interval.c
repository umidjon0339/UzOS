/*
 * sys_sched_rr_get_interval.c - sched_rr_get_interval(): the time slice of a process.
 *
 * The name says "RR" because the answer matters most for SCHED_RR: it
 * is how long a real-time process runs before the next one at its level
 * gets a turn. UzOS answers for every policy, from the same function the
 * scheduler uses (policy_slice_ticks), so it is never out of date:
 *
 *     SCHED_RR      10 ticks = 100 ms
 *     SCHED_OTHER    5 ticks =  50 ms (10 in the power-saver profile)
 *     SCHED_FIFO     0: a FIFO turn ends only when the process sleeps
 *
 * Uses: policy_slice_ticks (core/policy), timer_frequency (device_management),
 *       process_get_by_pid and the current process (process_management).
 */

#include "modules/cpu_management/syscalls/sched_rr_get_interval/sys_sched_rr_get_interval.h"
#include "modules/cpu_management/core/policy/policy.h"              // policy_slice_ticks
#include "modules/device_management/timer/timer.h"                  // timer_frequency
#include "modules/process_management/core/scheduler/scheduler.h"    // the current process

int sys_sched_rr_get_interval(uint32_t pid, timespec_t *interval) {
    process_t *p = pid ? process_get_by_pid(pid) : scheduler_get_current_process();
    uint32_t hz = timer_frequency();
    if (!p || !interval || !hz)
        return -1;

    uint32_t ms = policy_slice_ticks(p) * 1000 / hz;
    interval->tv_sec  = ms / 1000;
    interval->tv_nsec = (ms % 1000) * 1000000;
    return 0;
}

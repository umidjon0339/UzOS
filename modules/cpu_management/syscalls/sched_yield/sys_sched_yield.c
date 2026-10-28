/*
 * sys_sched_yield.c - sched_yield(): give the rest of this turn to the next process.
 *
 * The caller stays READY. The scheduler picks again (core/priority):
 * processes at the SAME rank go first (round robin), so yielding lets
 * your equals run; a lower-ranked process still waits. A SCHED_FIFO
 * process that yields goes to the back of its level, the only way it
 * ever lets an equal run.
 *
 * Uses: scheduler_yield (process_management/core/scheduler).
 */

#include "modules/cpu_management/syscalls/sched_yield/sys_sched_yield.h"   // this syscall
#include "modules/process_management/core/scheduler/scheduler.h"            // scheduler_yield

int sys_sched_yield(void) {
    scheduler_yield();
    return 0;
}

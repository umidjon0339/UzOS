/*
 * sys_getpriority.c - getpriority(): read the nice value, Linux style.
 *
 * Why "20 - nice" and not the nice value itself? Nice can be -1, and -1
 * is also what a failed syscall returns. So the raw syscall answers
 * 20 - nice, a number from 1 to 40 that is never negative; the C
 * library turns it back into the nice value. (Linux does exactly this.)
 *
 *     nice -20 -> 40     nice 0 -> 20     nice 19 -> 1     error -> -1
 *
 * Uses: the process table and the current process (process_management),
 *       NICE_MAX (core/priority).
 */

#include "modules/cpu_management/syscalls/getpriority/sys_getpriority.h"
#include "modules/cpu_management/core/priority/priority.h"          // NICE_MAX
#include "modules/process_management/core/scheduler/scheduler.h"    // the current process

#define ENCODE(nice) (20 - (nice))

int32_t sys_getpriority(int which, uint32_t who) {
    process_t *self = scheduler_get_current_process();
    if (!self)
        return -1;

    if (which == PRIO_PROCESS) {
        process_t *p = who ? process_get_by_pid(who) : self;
        return p ? ENCODE(p->nice) : -1;
    }
    if (which == PRIO_USER) {
        uint32_t uid = who ? who : self->uid;
        process_t *table = process_get_table();
        int best = NICE_MAX + 1;                // the lowest nice wins
        for (int i = 0; i < MAX_PROCESSES; i++) {
            process_t *p = &table[i];
            if (p->state != PROCESS_STATE_FREE && p->uid == uid && p->nice < best)
                best = p->nice;
        }
        return best <= NICE_MAX ? ENCODE(best) : -1;
    }
    return -1;          // PRIO_PGRP: UzOS has no process groups
}

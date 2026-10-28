/*
 * sys_setpriority.c - setpriority(): the Unix way to be nice.
 *
 *     setpriority(PRIO_PROCESS, 0, 10)      me: nice 10 (less CPU)
 *     setpriority(PRIO_PROCESS, 7, 5)       process 7: nice 5
 *     setpriority(PRIO_USER, 1000, 19)      every process of uid 1000
 *
 * Each change goes through priority_set_nice(), where the defence
 * priority_bounds checks the value and the caller's rights (users may
 * only raise nice, on their own processes).
 *
 * Uses: priority_set_nice (core/priority), the process table and the
 *       current process (process_management).
 */

#include "modules/cpu_management/syscalls/setpriority/sys_setpriority.h"
#include "modules/cpu_management/core/priority/priority.h"          // priority_set_nice
#include "modules/process_management/core/scheduler/scheduler.h"    // the current process

int32_t sys_setpriority(int which, uint32_t who, int nice) {
    process_t *self = scheduler_get_current_process();
    if (!self)
        return -1;

    if (which == PRIO_PROCESS)
        return priority_set_nice(self, who ? who : self->pid, nice) ? 0 : -1;

    if (which == PRIO_USER) {
        uint32_t uid = who ? who : self->uid;
        process_t *table = process_get_table();
        int found = 0, refused = 0;
        for (int i = 0; i < MAX_PROCESSES; i++) {
            process_t *p = &table[i];
            if (p->state == PROCESS_STATE_FREE || p->state == PROCESS_STATE_ZOMBIE ||
                p->uid != uid || p->is_system)
                continue;
            found = 1;
            refused |= !priority_set_nice(self, p->pid, nice);
        }
        return found && !refused ? 0 : -1;
    }
    return -1;          // PRIO_PGRP: UzOS has no process groups
}

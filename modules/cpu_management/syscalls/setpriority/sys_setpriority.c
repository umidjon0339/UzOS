/*
 * sys_setpriority.c - setpriority(): ask for a different priority level.
 *
 * Everything that matters happens in priority_set(), where the defence
 * priority_bounds checks the level and the caller's rights. Here we
 * only turn "pid 0" into "me".
 *
 * Uses: priority_set (core/priority), the current process
 *       (process_management).
 */

#include "sys_setpriority.h"
#include "priority.h"
#include "scheduler.h"

int32_t sys_setpriority(uint32_t pid, int32_t level) {
    process_t *self = scheduler_get_current_process();
    if (pid == 0)
        pid = self ? self->pid : 0;
    return priority_set(self, pid, level) ? 0 : -1;
}

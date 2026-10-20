/*
 * sys_getpriority.c - getpriority(): the level a process was given.
 *
 * Returns the BASE level, not the effective one: aging changes the
 * effective level every few ticks, so it would be stale at once.
 *
 * Uses: process_get_by_pid and the current process (process_management).
 */

#include "sys_getpriority.h"
#include "scheduler.h"

int32_t sys_getpriority(uint32_t pid) {
    process_t *p = pid ? process_get_by_pid(pid) : scheduler_get_current_process();
    return p ? p->priority : -1;
}

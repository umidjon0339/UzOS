/*
 * sys_getpid.c - getpid(): return the PID of the calling process.
 *
 * Uses: scheduler_get_current_process() (core/scheduler).
 */

#include "sys_getpid.h"
#include "scheduler.h"

uint32_t sys_getpid(void) {
    process_t *self = scheduler_get_current_process();
    return self ? self->pid : 0;
}

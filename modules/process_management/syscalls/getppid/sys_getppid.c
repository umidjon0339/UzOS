/*
 * sys_getppid.c - getppid(): return the caller's parent process id.
 * Uses: scheduler_get_current_process (core/scheduler), process_t.ppid.
 */
#include "sys_getppid.h"
#include "process.h"
#include "scheduler.h"

uint32_t sys_getppid(void) {
    process_t *self = scheduler_get_current_process();
    return self ? self->ppid : 0;
}

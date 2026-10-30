/*
 * sys_getppid.c - getppid(): return the caller's parent process id.
 * Uses: scheduler_get_current_process (core/scheduler), process_t.ppid.
 */
#include "modules/process_management/syscalls/getppid/sys_getppid.h"  // this file
#include "modules/process_management/core/process/process.h"          // process_t
#include "modules/process_management/core/scheduler/scheduler.h"      // scheduler_get_current_process

uint32_t sys_getppid(void) {
    process_t *self = scheduler_get_current_process();
    return self ? self->ppid : 0;
}

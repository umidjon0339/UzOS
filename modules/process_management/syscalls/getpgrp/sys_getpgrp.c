/*
 * sys_getpgrp.c - getpgrp(): the caller's process-group id.
 * Uses: scheduler_get_current_process (core/scheduler), process_t.pgid.
 */
#include "modules/process_management/syscalls/getpgrp/sys_getpgrp.h"  // this file
#include "modules/process_management/core/process/process.h"          // process_t
#include "modules/process_management/core/scheduler/scheduler.h"      // scheduler_get_current_process

uint32_t sys_getpgrp(void) {
    process_t *self = scheduler_get_current_process();
    return self ? self->pgid : 0;
}

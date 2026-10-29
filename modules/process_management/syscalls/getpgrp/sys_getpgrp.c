/*
 * sys_getpgrp.c - getpgrp(): the caller's process-group id.
 * Uses: scheduler_get_current_process (core/scheduler), process_t.pgid.
 */
#include "sys_getpgrp.h"
#include "process.h"
#include "scheduler.h"

uint32_t sys_getpgrp(void) {
    process_t *self = scheduler_get_current_process();
    return self ? self->pgid : 0;
}

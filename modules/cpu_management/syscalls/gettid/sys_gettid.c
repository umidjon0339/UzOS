/*
 * sys_gettid.c - gettid(): the id of this one thread.
 *
 *   getpid()   the PROCESS (thread group): the same for all its threads
 *   gettid()   the THREAD: different for each
 *
 * In UzOS every thread is a slot in the process table, so a thread's id
 * is simply its pid there. For the first thread of a group the two
 * answers are equal (tid == tgid).
 *
 * Uses: scheduler_get_current_process (process_management).
 */

#include "modules/cpu_management/syscalls/gettid/sys_gettid.h"
#include "modules/process_management/core/scheduler/scheduler.h"    // the current process

uint32_t sys_gettid(void) {
    process_t *self = scheduler_get_current_process();
    return self ? self->pid : 0;
}

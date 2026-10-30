/*
 * sys_getsid.c - getsid(pid): return a process's session id.
 * Uses: scheduler_get_current_process, process_get_by_pid (core/process).
 */
#include "modules/process_management/syscalls/getsid/sys_getsid.h"  // this file
#include "modules/process_management/core/process/process.h"        // process_t, process_get_by_pid
#include "modules/process_management/core/scheduler/scheduler.h"    // scheduler_get_current_process

int sys_getsid(uint32_t pid) {
    process_t *t = pid ? process_get_by_pid(pid) : scheduler_get_current_process();
    return t ? (int)t->sid : -1;
}

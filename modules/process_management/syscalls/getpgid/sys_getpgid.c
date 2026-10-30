/*
 * sys_getpgid.c - getpgid(pid): return a process's group id.
 * Uses: scheduler_get_current_process, process_get_by_pid (core/process).
 */
#include "modules/process_management/syscalls/getpgid/sys_getpgid.h"  // this file
#include "modules/process_management/core/process/process.h"          // process_t, process_get_by_pid
#include "modules/process_management/core/scheduler/scheduler.h"      // scheduler_get_current_process

int sys_getpgid(uint32_t pid) {
    process_t *t = pid ? process_get_by_pid(pid) : scheduler_get_current_process();
    return t ? (int)t->pgid : -1;
}

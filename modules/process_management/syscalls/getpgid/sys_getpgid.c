/*
 * sys_getpgid.c - getpgid(pid): return a process's group id.
 * Uses: scheduler_get_current_process, process_get_by_pid (core/process).
 */
#include "sys_getpgid.h"
#include "process.h"
#include "scheduler.h"

int sys_getpgid(uint32_t pid) {
    process_t *t = pid ? process_get_by_pid(pid) : scheduler_get_current_process();
    return t ? (int)t->pgid : -1;
}

/*
 * sys_getsid.c - getsid(pid): return a process's session id.
 * Uses: scheduler_get_current_process, process_get_by_pid (core/process).
 */
#include "sys_getsid.h"
#include "process.h"
#include "scheduler.h"

int sys_getsid(uint32_t pid) {
    process_t *t = pid ? process_get_by_pid(pid) : scheduler_get_current_process();
    return t ? (int)t->sid : -1;
}

/*
 * sys_sched_getaffinity.c - sched_getaffinity(): read a process's CPU mask.
 *
 * Like Linux, the raw syscall returns how many bytes of mask it wrote,
 * and the rest of the caller's buffer is left alone.
 *
 * Uses: affinity_get (core/affinity), process_get_by_pid and the
 *       current process (process_management).
 */

#include "modules/cpu_management/syscalls/sched_getaffinity/sys_sched_getaffinity.h"
#include "modules/cpu_management/core/affinity/affinity.h"          // affinity_get
#include "modules/process_management/core/scheduler/scheduler.h"    // the current process

int sys_sched_getaffinity(uint32_t pid, uint32_t size, uint8_t *mask) {
    process_t *p = pid ? process_get_by_pid(pid) : scheduler_get_current_process();
    if (!p || !mask || size < sizeof(uint32_t))
        return -1;

    uint32_t bits = affinity_get(p);
    for (int i = 0; i < 4; i++)
        mask[i] = (bits >> (8 * i)) & 0xFF;
    return sizeof(uint32_t);
}

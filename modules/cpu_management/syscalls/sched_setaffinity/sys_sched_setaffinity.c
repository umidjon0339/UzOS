/*
 * sys_sched_setaffinity.c - sched_setaffinity(): pin a process to CPUs.
 *
 * The mask comes as bytes (Linux's cpu_set_t can be 128 bytes for 1024
 * CPUs). UzOS reads the first 4 bytes = CPUs 0-31; any bit set further
 * on names a CPU that does not exist, so the call is refused.
 *
 * Uses: affinity_set (core/affinity), the current process (process_management).
 */

#include "modules/cpu_management/syscalls/sched_setaffinity/sys_sched_setaffinity.h"
#include "modules/cpu_management/core/affinity/affinity.h"          // affinity_set
#include "modules/process_management/core/scheduler/scheduler.h"    // the current process

int sys_sched_setaffinity(uint32_t pid, uint32_t size, const uint8_t *mask) {
    process_t *self = scheduler_get_current_process();
    if (!mask || size == 0 || !self)
        return -1;

    uint32_t bits = 0;
    for (uint32_t i = 0; i < size; i++) {
        if (i < 4)
            bits |= (uint32_t)mask[i] << (8 * i);
        else if (mask[i])
            return -1;                      // CPU 32 or above: no such CPU
    }
    return affinity_set(self, pid ? pid : self->pid, bits) ? 0 : -1;
}

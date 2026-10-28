/*
 * sys_membarrier.c - membarrier(): make every thread see my writes.
 *
 * CPUs may reorder memory accesses. Threads that share data put a
 * BARRIER between their accesses ("everything before is visible before
 * anything after"). membarrier() lets ONE thread do it for ALL: after
 * it returns, every other running thread has passed a barrier too. That
 * makes the fast path of the other threads cheaper (no barrier there).
 *
 * How the kernel keeps the promise:
 *   many CPUs   interrupt every CPU running a thread of this process,
 *               each runs a barrier in the interrupt (expedited), or wait
 *               until every CPU has switched tasks once (global)
 *   UzOS        one CPU: every OTHER thread is not running right now,
 *               and a task switch is itself a full barrier. So one
 *               barrier here, on this CPU, is enough.
 *
 * PRIVATE_EXPEDITED must be registered first (REGISTER_PRIVATE_EXPEDITED),
 * as in Linux; unregistered use fails.
 *
 * Uses: the current process (process_management).
 */

#include "modules/cpu_management/syscalls/membarrier/sys_membarrier.h"
#include "modules/process_management/core/scheduler/scheduler.h"    // the current process

#define SUPPORTED (MEMBARRIER_CMD_GLOBAL | MEMBARRIER_CMD_GLOBAL_EXPEDITED | \
                   MEMBARRIER_CMD_REGISTER_GLOBAL_EXPEDITED |                \
                   MEMBARRIER_CMD_PRIVATE_EXPEDITED |                        \
                   MEMBARRIER_CMD_REGISTER_PRIVATE_EXPEDITED)

// A full memory barrier. 'lock or' works on every x86 (mfence needs SSE2).
static void barrier(void) {
    asm volatile("lock orl $0, (%%esp)" : : : "memory");
}

int sys_membarrier(int cmd, uint32_t flags, int cpu_id) {
    process_t *self = scheduler_get_current_process();
    (void)cpu_id;
    if (flags != 0 || !self)
        return -1;

    switch (cmd) {
        case MEMBARRIER_CMD_QUERY:
            return SUPPORTED;
        case MEMBARRIER_CMD_REGISTER_PRIVATE_EXPEDITED: {
            // For the whole process: every thread of the group.
            process_t *table = process_get_table();
            for (int i = 0; i < MAX_PROCESSES; i++)
                if (table[i].state != PROCESS_STATE_FREE && table[i].tgid == self->tgid)
                    table[i].membarrier_registered = 1;
            return 0;
        }
        case MEMBARRIER_CMD_REGISTER_GLOBAL_EXPEDITED:
            return 0;                           // nothing to prepare on one CPU
        case MEMBARRIER_CMD_PRIVATE_EXPEDITED:
            if (!self->membarrier_registered)
                return -1;                      // Linux: EPERM
            barrier();
            return 0;
        case MEMBARRIER_CMD_GLOBAL:
        case MEMBARRIER_CMD_GLOBAL_EXPEDITED:
            barrier();
            return 0;
        default:
            return -1;
    }
}

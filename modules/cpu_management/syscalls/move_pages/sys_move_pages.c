/*
 * sys_move_pages.c - move_pages(): ask about (or move) single pages.
 *
 *     void *pages[1] = { buffer };
 *     int status[1];
 *     move_pages(0, 1, pages, NULL, status, 0);   // status[0] = node of buffer
 *
 * With one node every mapped page is already on node 0: "moving" it to
 * node 0 succeeds with nothing to do, moving it anywhere else fails
 * with ENODEV for that page. The answers are still real: an unmapped
 * address really is reported as ENOENT.
 *
 * Uses: numa_node_of_address / numa_online_nodes (core/numa),
 *       process_get_by_pid and the current process (process_management),
 *       caps_has (security_management) for other users' processes.
 */

#include "modules/cpu_management/syscalls/move_pages/sys_move_pages.h"
#include "modules/cpu_management/core/numa/numa.h"                  // numa_node_of_address
#include "modules/security_management/core/caps/caps.h"             // caps_has
#include "modules/process_management/core/scheduler/scheduler.h"    // the current process
#include "modules/memory_management/core/layout/memory_layout.h"     // USER_SPACE_*

#define ENOENT 2        // Linux error numbers, returned negative per page
#define EFAULT 14
#define ENODEV 19

int sys_move_pages(uint32_t pid, uint32_t count, const uint32_t *pages,
                   const int32_t *nodes, int32_t *status, uint32_t flags) {
    process_t *self = scheduler_get_current_process();
    process_t *target = pid ? process_get_by_pid(pid) : self;

    if (!target || !pages || !status || (flags & ~(MPOL_MF_MOVE | MPOL_MF_MOVE_ALL)))
        return -1;
    if (target->uid != self->uid && !caps_has(self->uid, CAP_PRIORITY))
        return -1;

    for (uint32_t i = 0; i < count; i++) {
        uint32_t addr = pages[i];
        int node = numa_node_of_address(addr);

        if (node < 0)
            status[i] = (addr < USER_SPACE_START || addr >= USER_SPACE_END) ? -EFAULT : -ENOENT;
        else if (nodes && ((uint32_t)nodes[i] >= 32 || !(numa_online_nodes() & (1u << nodes[i]))))
            status[i] = -ENODEV;            // no such node to move it to
        else
            status[i] = node;               // where it is (and stays: node 0)
    }
    return 0;
}

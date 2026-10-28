/*
 * sys_mbind.c - mbind(): "this buffer should live on node 1".
 *
 * set_mempolicy() sets a policy for all of a process's memory; mbind()
 * sets one for a single range, e.g. a big array that one CPU group
 * works on. Rules (as in Linux):
 *   - addr is page aligned, the range is mapped user memory
 *   - mode and nodes form a valid policy (core/numa)
 *   - MPOL_MF_STRICT: fail if a page is already on a node outside the
 *     mask (never happens with one node)
 *   - MPOL_MF_MOVE: move such pages (none to move with one node)
 * UzOS remembers up to 4 ranges per process; get_mempolicy(MPOL_F_ADDR)
 * reads them back.
 *
 * Uses: numa_* (core/numa), the current process (process_management),
 *       PAGE_SIZE (memory_layout.h).
 */

#include "modules/cpu_management/syscalls/mbind/sys_mbind.h"
#include "modules/cpu_management/core/numa/numa.h"                  // numa_*
#include "modules/memory_management/core/layout/memory_layout.h"     // PAGE_SIZE
#include "modules/process_management/core/scheduler/scheduler.h"    // the current process

int sys_mbind(uint32_t addr, uint32_t len, int mode, const uint32_t *nodemask,
              uint32_t maxnode, uint32_t flags) {
    process_t *self = scheduler_get_current_process();
    uint32_t nodes = (nodemask && maxnode) ? *nodemask : 0;

    if (!self || len == 0 || addr % PAGE_SIZE != 0)
        return -1;
    if (flags & ~(MPOL_MF_STRICT | MPOL_MF_MOVE | MPOL_MF_MOVE_ALL))
        return -1;
    if (maxnode && maxnode < 32)
        nodes &= (1u << maxnode) - 1;       // only the first maxnode bits count
    if (!numa_policy_ok(mode, nodes))
        return -1;

    // Every page of the range must be mapped user memory.
    for (uint32_t page = addr; page - addr < len; page += PAGE_SIZE) {
        int node = numa_node_of_address(page);
        if (node < 0)
            return -1;
        if ((flags & MPOL_MF_STRICT) && nodes && !(nodes & (1u << node)))
            return -1;                      // on a node it should not be on
        if (page + PAGE_SIZE < page)
            break;                          // the end of the address space
    }
    return numa_bind_range(self, addr, len, mode, nodes) ? 0 : -1;
}

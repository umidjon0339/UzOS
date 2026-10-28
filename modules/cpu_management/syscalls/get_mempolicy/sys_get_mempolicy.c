/*
 * sys_get_mempolicy.c - get_mempolicy(): four questions in one call.
 *
 *   get_mempolicy(&mode, &nodes, 32, 0, 0)                   my policy
 *   get_mempolicy(&mode, &nodes, 32, buf, MPOL_F_ADDR)       policy of buf (mbind)
 *   get_mempolicy(&node, 0, 0, buf, MPOL_F_NODE|MPOL_F_ADDR) node of buf's page
 *   get_mempolicy(0, &nodes, 32, 0, MPOL_F_MEMS_ALLOWED)     nodes I may use
 *
 * Uses: numa_* (core/numa), the current process (process_management).
 */

#include "modules/cpu_management/syscalls/get_mempolicy/sys_get_mempolicy.h"
#include "modules/cpu_management/core/numa/numa.h"                  // numa_*
#include "modules/process_management/core/scheduler/scheduler.h"    // the current process

int sys_get_mempolicy(int32_t *mode, uint32_t *nodemask, uint32_t maxnode,
                      uint32_t addr, uint32_t flags) {
    process_t *self = scheduler_get_current_process();
    int m = 0;
    uint32_t nodes = 0;

    if (!self || (flags & ~(MPOL_F_NODE | MPOL_F_ADDR | MPOL_F_MEMS_ALLOWED)))
        return -1;
    if (nodemask && maxnode < NUMA_NODES)
        return -1;                          // the mask would not fit

    if (flags & MPOL_F_MEMS_ALLOWED) {
        if (flags != MPOL_F_MEMS_ALLOWED)
            return -1;                      // must be asked alone
        nodes = numa_online_nodes();
    } else if (flags & MPOL_F_ADDR) {
        if (flags & MPOL_F_NODE) {
            m = numa_node_of_address(addr); // which node holds this page
            if (m < 0)
                return -1;
        } else {
            numa_policy_at(self, addr, &m, &nodes);
        }
    } else if (flags & MPOL_F_NODE) {
        return -1;                          // MPOL_F_NODE needs an address in UzOS
    } else {
        m = self->mempolicy;
        nodes = self->mempolicy_nodes;
    }

    if (mode)
        *mode = m;
    if (nodemask)
        *nodemask = nodes;
    return 0;
}

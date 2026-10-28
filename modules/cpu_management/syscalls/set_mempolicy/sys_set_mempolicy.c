/*
 * sys_set_mempolicy.c - set_mempolicy(): the process's memory policy.
 *
 *     uint32_t nodes = 1 << 0;
 *     set_mempolicy(MPOL_BIND, &nodes, 32);   // only node 0
 *     set_mempolicy(MPOL_DEFAULT, NULL, 0);   // back to the default
 *
 * Children inherit it (core/task). It is what `numactl --membind=0`
 * sets before starting a program.
 *
 * Uses: numa_policy_ok (core/numa), the current process (process_management).
 */

#include "modules/cpu_management/syscalls/set_mempolicy/sys_set_mempolicy.h"
#include "modules/cpu_management/core/numa/numa.h"                  // numa_policy_ok
#include "modules/process_management/core/scheduler/scheduler.h"    // the current process

int sys_set_mempolicy(int mode, const uint32_t *nodemask, uint32_t maxnode) {
    process_t *self = scheduler_get_current_process();
    uint32_t nodes = (nodemask && maxnode) ? *nodemask : 0;

    if (maxnode && maxnode < 32)
        nodes &= (1u << maxnode) - 1;
    if (!self || !numa_policy_ok(mode, nodes))
        return -1;

    self->mempolicy = mode;
    self->mempolicy_nodes = nodes;
    return 0;
}

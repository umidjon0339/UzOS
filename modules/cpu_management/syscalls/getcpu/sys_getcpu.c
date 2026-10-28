/*
 * sys_getcpu.c - getcpu(): where am I running right now?
 *
 * On a multi-CPU machine the answer can be out of date the moment it
 * returns (the scheduler may move the process). It is a hint, used for
 * per-CPU data. UzOS runs on CPU 0, which is on node 0.
 *
 * Uses: affinity_current_cpu (core/affinity), numa_node_of_cpu (core/numa).
 */

#include "modules/cpu_management/syscalls/getcpu/sys_getcpu.h"
#include "modules/cpu_management/core/affinity/affinity.h"      // affinity_current_cpu
#include "modules/cpu_management/core/numa/numa.h"              // numa_node_of_cpu

int sys_getcpu(uint32_t *cpu, uint32_t *node) {
    int now = affinity_current_cpu();
    if (cpu)
        *cpu = now;
    if (node)
        *node = numa_node_of_cpu(now);
    return 0;
}

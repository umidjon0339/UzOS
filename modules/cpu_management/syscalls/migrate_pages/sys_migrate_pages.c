/*
 * sys_migrate_pages.c - migrate_pages(): a whole process moves house.
 *
 * Used when a program is moved to CPUs of another node: its memory
 * should follow it. With one node, old and new must both be node 0,
 * so every page is already where it belongs and the answer is 0 (no
 * page failed). Masks naming a node that does not exist are refused.
 *
 * Uses: numa_online_nodes (core/numa), process_get_by_pid and the
 *       current process (process_management), caps_has (security_management).
 */

#include "modules/cpu_management/syscalls/migrate_pages/sys_migrate_pages.h"
#include "modules/cpu_management/core/numa/numa.h"                  // numa_online_nodes
#include "modules/security_management/core/caps/caps.h"             // caps_has
#include "modules/process_management/core/scheduler/scheduler.h"    // the current process

int sys_migrate_pages(uint32_t pid, uint32_t maxnode, const uint32_t *old_nodes,
                      const uint32_t *new_nodes) {
    process_t *self = scheduler_get_current_process();
    process_t *target = pid ? process_get_by_pid(pid) : self;

    if (!target || !old_nodes || !new_nodes || maxnode == 0)
        return -1;
    if (target->uid != self->uid && !caps_has(self->uid, CAP_PRIORITY))
        return -1;

    uint32_t mask = maxnode < 32 ? (1u << maxnode) - 1 : 0xFFFFFFFFu;
    if ((*old_nodes & mask & ~numa_online_nodes()) || (*new_nodes & mask & ~numa_online_nodes()))
        return -1;                          // a node that does not exist
    if ((*new_nodes & mask) == 0)
        return -1;                          // nowhere to move to
    return 0;                               // every page is on node 0 already
}

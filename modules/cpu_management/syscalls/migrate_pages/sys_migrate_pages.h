#ifndef SYS_MIGRATE_PAGES_H
#define SYS_MIGRATE_PAGES_H

/*
 * sys_migrate_pages.h - migrate_pages(): move all of a process's pages between nodes.
 */

#include <stdint.h>

#define SYS_MIGRATE_PAGES 49    // EAX = 49, EBX = pid, ECX = maxnode, EDX = &old_nodes, ESI = &new_nodes

// Move every page of 'pid' that is on a node in *old_nodes to the
// nodes in *new_nodes. Returns how many pages could NOT be moved
// (0 = all done), or -1 for a bad pid / node mask.
int sys_migrate_pages(uint32_t pid, uint32_t maxnode, const uint32_t *old_nodes,
                      const uint32_t *new_nodes);

#endif

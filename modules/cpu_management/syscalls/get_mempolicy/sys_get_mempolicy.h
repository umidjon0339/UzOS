#ifndef SYS_GET_MEMPOLICY_H
#define SYS_GET_MEMPOLICY_H

/*
 * sys_get_mempolicy.h - get_mempolicy(): read a memory policy (or a page's node).
 */

#include <stdint.h>

#define SYS_GET_MEMPOLICY 48    // EAX = 48, EBX = &mode, ECX = &nodemask, EDX = maxnode, ESI = addr, EDI = flags

// flags 0:                         *mode / *nodemask = the process's policy
// MPOL_F_ADDR:                     the policy in force at 'addr'
// MPOL_F_NODE | MPOL_F_ADDR:       *mode = the node holding the page at 'addr'
// MPOL_F_MEMS_ALLOWED:             *nodemask = the nodes this process may use
// 'mode' and 'nodemask' may be NULL. 0 = done, -1 = error.
int sys_get_mempolicy(int32_t *mode, uint32_t *nodemask, uint32_t maxnode,
                      uint32_t addr, uint32_t flags);

#endif

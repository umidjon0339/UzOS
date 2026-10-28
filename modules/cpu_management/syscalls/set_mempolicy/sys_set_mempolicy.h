#ifndef SYS_SET_MEMPOLICY_H
#define SYS_SET_MEMPOLICY_H

/*
 * sys_set_mempolicy.h - set_mempolicy(): where my memory should come from.
 */

#include <stdint.h>

#define SYS_SET_MEMPOLICY 47    // EAX = 47, EBX = mode, ECX = &nodemask, EDX = maxnode

// The caller's memory policy becomes 'mode' (MPOL_*, core/numa) on the
// nodes in *nodemask. 0 = done, -1 = not a valid policy.
int sys_set_mempolicy(int mode, const uint32_t *nodemask, uint32_t maxnode);

#endif

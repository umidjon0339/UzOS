#ifndef SYS_MBIND_H
#define SYS_MBIND_H

/*
 * sys_mbind.h - mbind(): a memory policy for one range of my memory.
 */

#include <stdint.h>

#define SYS_MBIND 46    // EAX = 46, EBX = addr, ECX = len, EDX = mode, ESI = &nodemask, EDI = maxnode, EBP = flags

// Pages [addr, addr + len) of the caller follow 'mode' (MPOL_*, core/numa)
// on the nodes in *nodemask (maxnode bits). flags: MPOL_MF_STRICT /
// MPOL_MF_MOVE. 0 = done, -1 = refused.
int sys_mbind(uint32_t addr, uint32_t len, int mode, const uint32_t *nodemask,
              uint32_t maxnode, uint32_t flags);

#endif

#ifndef SYS_MEMBARRIER_H
#define SYS_MEMBARRIER_H

/*
 * sys_membarrier.h - membarrier(): a memory barrier on every thread at once.
 */

#include <stdint.h>

#define SYS_MEMBARRIER 50   // EAX = 50, EBX = cmd, ECX = flags (0), EDX = cpu_id (unused)

// Commands (Linux values; each is one bit)
#define MEMBARRIER_CMD_QUERY                       0
#define MEMBARRIER_CMD_GLOBAL                      (1 << 0)
#define MEMBARRIER_CMD_GLOBAL_EXPEDITED            (1 << 1)
#define MEMBARRIER_CMD_REGISTER_GLOBAL_EXPEDITED   (1 << 2)
#define MEMBARRIER_CMD_PRIVATE_EXPEDITED           (1 << 3)
#define MEMBARRIER_CMD_REGISTER_PRIVATE_EXPEDITED  (1 << 4)

// QUERY: the set of supported commands. REGISTER_*: 0. A barrier: 0.
// -1 for an unknown command, or PRIVATE_EXPEDITED before registering.
int sys_membarrier(int cmd, uint32_t flags, int cpu_id);

#endif

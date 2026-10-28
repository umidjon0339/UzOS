#ifndef SYS_GETCPU_H
#define SYS_GETCPU_H

/*
 * sys_getcpu.h - getcpu(): which CPU and NUMA node am I running on?
 */

#include <stdint.h>

#define SYS_GETCPU 40   // EAX = 40, EBX = &cpu (or 0), ECX = &node (or 0)

// Write the current CPU number to *cpu and its NUMA node to *node
// (either pointer may be NULL). Always 0 = done.
int sys_getcpu(uint32_t *cpu, uint32_t *node);

#endif

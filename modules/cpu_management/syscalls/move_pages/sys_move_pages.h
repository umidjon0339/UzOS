#ifndef SYS_MOVE_PAGES_H
#define SYS_MOVE_PAGES_H

/*
 * sys_move_pages.h - move_pages(): where are these pages, and move them.
 */

#include <stdint.h>

#define SYS_MOVE_PAGES 45   // EAX = 45, EBX = pid, ECX = count, EDX = &pages, ESI = &nodes (or 0), EDI = &status, EBP = flags

// For each of the 'count' addresses in pages[]: if nodes is NULL,
// status[i] = the node holding that page; else move it to nodes[i]
// and status[i] = where it is now. A negative status is an error for
// that page: -2 (ENOENT) not mapped, -14 (EFAULT) not user memory,
// -19 (ENODEV) no such node. Returns 0, or -1 for a bad pid / flags.
int sys_move_pages(uint32_t pid, uint32_t count, const uint32_t *pages,
                   const int32_t *nodes, int32_t *status, uint32_t flags);

#endif

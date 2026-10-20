#ifndef DEF_MMAP_OWNERSHIP_H
#define DEF_MMAP_OWNERSHIP_H

#include <stdint.h>

// Is [start, start + pages) exactly one region that mmap() handed out?
// 1 = yes, 0 = refused (logged).
int def_munmap_allowed(uint32_t start, uint32_t pages);

#endif

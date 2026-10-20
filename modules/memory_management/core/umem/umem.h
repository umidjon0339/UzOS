#ifndef UMEM_H
#define UMEM_H

/*
 * umem.h - User memory: pages that ring 3 programs may use.
 *
 * sbrk() and mmap() both come down to "give the program N fresh pages at
 * this address". umem does that (and the reverse), and keeps the list of
 * regions mmap() handed out, so munmap() can check it.
 */

#include <stdint.h>

#define UMEM_MAX_REGIONS 32

// Map 'pages' zeroed, user-writable pages at 'start'.
// All or nothing: on failure nothing stays mapped. Returns 1 on success.
int umem_map(uint32_t start, uint32_t pages);

// Unmap 'pages' pages at 'start' and give their frames back to the PMM.
void umem_unmap(uint32_t start, uint32_t pages);

/* ---------- mmap regions ---------- */

// Find room for 'pages' pages in the mmap area and record it.
// Returns the start address, or 0 if the area is full.
uint32_t umem_region_add(uint32_t pages);

// Size in pages of the region that starts exactly at 'start', or 0.
uint32_t umem_region_pages(uint32_t start);

// Forget the region that starts at 'start'.
void umem_region_remove(uint32_t start);

#endif

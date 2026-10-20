#ifndef PMM_H
#define PMM_H

/*
 * pmm.h - Physical Memory Manager: hands out 4 KiB frames of real RAM.
 *
 * RAM is cut into "frames" of PAGE_SIZE bytes. One bit per frame says
 * whether it is in use (a "bitmap"). The VMM maps frames to virtual
 * pages; the PMM only knows which physical frames are free.
 */

#include <stdint.h>
#include "multiboot.h"
#include "memory_layout.h"

#define PMM_BLOCK_SIZE  PAGE_SIZE
#define PMM_MAX_FRAMES  (0x100000000ULL / PAGE_SIZE)    // enough for 4 GiB
#define PMM_MAX_REGIONS 16

// One entry of the bootloader's memory map, kept for `meminfo`.
typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type;      // MULTIBOOT_MEMORY_AVAILABLE, _RESERVED, ...
} pmm_region_t;

// Read the bootloader's memory map: free RAM becomes allocatable, and
// the kernel, the boot data and firmware areas are reserved for good.
void pmm_init(multiboot_info_t *mb_info);

// One free frame (its physical address), or NULL if RAM is full.
void *pmm_alloc_block(void);

// Give a frame back. Bad frees are refused (see pmm_free_guard).
void pmm_free_block(void *frame);

uint32_t pmm_get_max_blocks(void);     // frames of RAM in total
uint32_t pmm_get_used_blocks(void);
uint32_t pmm_get_free_blocks(void);

int pmm_is_used(uint32_t frame);       // frame = address / PAGE_SIZE
int pmm_is_reserved(uint32_t frame);   // kernel, firmware, boot data

int pmm_region_count(void);
const pmm_region_t *pmm_region(int index);

#endif

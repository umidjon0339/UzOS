/*
 * umem.c - Map and unmap user pages; remember mmap regions.
 *
 * Uses: pmm_alloc_block / pmm_free_block (core/pmm),
 *       vmm_map_page / vmm_unmap_page / vmm_walk (core/vmm), memset (lib).
 *
 * All processes share one address space for now, so there is one list
 * of mmap regions for the whole system.
 */

#include "umem.h"
#include "pmm.h"
#include "vmm.h"
#include "string.h"

typedef struct {
    uint32_t start;         // 0 = unused slot
    uint32_t pages;
} region_t;

static region_t regions[UMEM_MAX_REGIONS];

/* ---------- Pages ---------- */

void umem_unmap(uint32_t start, uint32_t pages) {
    for (uint32_t i = 0; i < pages; i++) {
        uint32_t virt = start + i * PAGE_SIZE;
        vmm_walk_t w;
        if (vmm_walk(virt, &w)) {
            vmm_unmap_page(virt);
            pmm_free_block((void *)(w.physical & ~(PAGE_SIZE - 1)));
        }
    }
}

int umem_map(uint32_t start, uint32_t pages) {
    for (uint32_t i = 0; i < pages; i++) {
        uint32_t virt = start + i * PAGE_SIZE;
        void *frame = pmm_alloc_block();

        if (!frame || !vmm_map_page(virt, (uint32_t)frame, PAGE_WRITABLE | PAGE_USER)) {
            if (frame)
                pmm_free_block(frame);
            umem_unmap(start, i);       // undo the pages already mapped
            return 0;
        }
        memset((void *)virt, 0, PAGE_SIZE);     // never hand out old data
    }
    return 1;
}

/* ---------- mmap regions ---------- */

// If [start, start + pages) overlaps a recorded region, return where that
// region ends (the next place worth trying). Return 0 if the range is free.
static uint32_t blocking_region_end(uint32_t start, uint32_t pages) {
    uint32_t end = start + pages * PAGE_SIZE;
    for (int i = 0; i < UMEM_MAX_REGIONS; i++) {
        region_t *r = &regions[i];
        uint32_t r_end = r->start + r->pages * PAGE_SIZE;
        if (r->start && start < r_end && r->start < end)
            return r_end;
    }
    return 0;
}

uint32_t umem_region_add(uint32_t pages) {
    // Find an empty slot in the list.
    region_t *slot = 0;
    for (int i = 0; i < UMEM_MAX_REGIONS && !slot; i++)
        if (!regions[i].start)
            slot = &regions[i];
    if (!slot || pages == 0 || pages > (MMAP_END - MMAP_START) / PAGE_SIZE)
        return 0;

    // First fit: try the lowest address; if a region is in the way, jump past it.
    uint32_t addr = MMAP_START;
    while (addr + pages * PAGE_SIZE <= MMAP_END) {
        uint32_t blocked_until = blocking_region_end(addr, pages);
        if (!blocked_until) {
            slot->start = addr;
            slot->pages = pages;
            return addr;
        }
        addr = blocked_until;
    }
    return 0;
}

uint32_t umem_region_pages(uint32_t start) {
    for (int i = 0; i < UMEM_MAX_REGIONS; i++)
        if (start && regions[i].start == start)
            return regions[i].pages;
    return 0;
}

void umem_region_remove(uint32_t start) {
    for (int i = 0; i < UMEM_MAX_REGIONS; i++)
        if (start && regions[i].start == start)
            regions[i].start = 0;
}

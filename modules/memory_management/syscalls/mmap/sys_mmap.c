/*
 * sys_mmap.c - mmap(): map a new region of zeroed pages for the caller.
 *
 * Real mmap() also maps files and takes protection flags. UzOS keeps
 * the core idea: "give me N bytes of fresh memory, you pick where".
 * The region goes in the mmap area (MMAP_START..MMAP_END) and is
 * recorded, so only munmap() of exactly this region can remove it.
 *
 * Uses: umem_region_add + umem_map (core/umem), and the defence
 *       mmap_page_count to turn 'length' into pages safely.
 */

#include "sys_mmap.h"
#include "umem.h"
#include "cpu.h"
#include "def_mmap_page_count.h"

void *sys_mmap(uint32_t length) {
    uint32_t pages;

    // DEFENCE mmap_page_count: sane length, page count without overflow.
    if (!def_mmap_page_count(length, &pages))
        return 0;

    uint32_t irq = cpu_irq_save();

    uint32_t start = umem_region_add(pages);
    if (start && !umem_map(start, pages)) {
        umem_region_remove(start);      // not enough RAM: undo the record
        start = 0;
    }

    cpu_irq_restore(irq);
    return (void *)start;
}

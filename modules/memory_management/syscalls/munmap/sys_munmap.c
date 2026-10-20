/*
 * sys_munmap.c - munmap(): unmap a region and free its frames.
 *
 * Only a region that mmap() handed out, as a whole, can be unmapped.
 * Without that rule, munmap(<kernel address>) would free frames the
 * kernel is still using.
 *
 * Uses: umem_unmap + umem_region_remove (core/umem), and the defences
 *       mmap_page_count and mmap_ownership.
 */

#include "sys_munmap.h"
#include "umem.h"
#include "cpu.h"
#include "def_mmap_page_count.h"
#include "def_mmap_ownership.h"

int sys_munmap(void *addr, uint32_t length) {
    uint32_t pages;
    uint32_t start = (uint32_t)addr;

    if (!def_mmap_page_count(length, &pages))
        return -1;

    uint32_t irq = cpu_irq_save();
    int result = -1;

    // DEFENCE mmap_ownership: exactly a region mmap() gave out.
    if (def_munmap_allowed(start, pages)) {
        umem_unmap(start, pages);
        umem_region_remove(start);
        result = 0;
    }

    cpu_irq_restore(irq);
    return result;
}

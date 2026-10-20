/*
 * def_mmap_ownership.c - DEFENCE mmap_ownership: munmap only what mmap gave.
 *
 * Stops:  munmap_kernel (security/attack/munmap_kernel)
 * Called: sys_munmap() (syscalls/munmap), before anything is unmapped.
 * Rule:   'start' is the start of a region mmap() recorded, and 'pages'
 *         is exactly that region's size.
 * Why:    munmap() frees the frames behind the pages. If any address were
 *         accepted, munmap(0x100000, 4096) would free the frame holding
 *         the kernel's own code, and the next allocation would overwrite
 *         it. Only memory you got from mmap() is yours to give back.
 */

#include "def_mmap_ownership.h"
#include "security_lab.h"
#include "umem.h"

// The whole rule, as a pure function so the selftest can try it.
static int owns_region(uint32_t recorded_pages, uint32_t pages) {
    return recorded_pages != 0 && recorded_pages == pages;
}

int def_munmap_allowed(uint32_t start, uint32_t pages) {
    if (owns_region(umem_region_pages(start), pages))
        return 1;
    sec_log("mmap_ownership", "munmap refused: not a region mmap() handed out");
    return 0;
}

static int selftest(void) {
    if (!owns_region(3, 3)) return 0;       // the exact region: ok
    if (owns_region(0, 1)) return 0;        // never mapped (kernel, heap, ...)
    if (owns_region(3, 1)) return 0;        // only part of a region
    if (owns_region(3, 5)) return 0;        // more than the region
    return 1;
}

static const defence_t defence = {
    "mmap_ownership",
    "munmap only exact regions that mmap handed out",
    "munmap_kernel",
    selftest
};

void def_mmap_ownership_register(void) {
    proc_security_register_defence(&defence);
}

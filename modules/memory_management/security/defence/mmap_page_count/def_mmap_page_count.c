/*
 * def_mmap_page_count.c - DEFENCE mmap_page_count: safe length -> pages.
 *
 * Stops:  mmap_length_wrap (security/attack/mmap_length_wrap)
 * Called: sys_mmap() and sys_munmap() (syscalls/), before anything else.
 * Rules:  1. 0 < length <= MMAP_MAX_BYTES
 *         2. pages = length / PAGE_SIZE, + 1 if there is a remainder
 * Why:    the usual "(length + PAGE_SIZE - 1) / PAGE_SIZE" overflows for a
 *         length near 4 GiB and gives 0 pages: mmap would "succeed" with
 *         no memory behind it. Rule 2 has no addition that can overflow,
 *         and rule 1 stops one call from asking for all of RAM.
 */

#include "def_mmap_page_count.h"
#include "security_lab.h"
#include "memory_layout.h"

// The whole rule, as a pure function so the selftest can try it.
static int page_count(uint32_t length, uint32_t *pages) {
    if (length == 0 || length > MMAP_MAX_BYTES) return 0;
    *pages = length / PAGE_SIZE + (length % PAGE_SIZE != 0);
    return 1;
}

int def_mmap_page_count(uint32_t length, uint32_t *pages) {
    if (page_count(length, pages))
        return 1;
    sec_log("mmap_page_count", "mmap length is 0 or too large");
    return 0;
}

static int selftest(void) {
    uint32_t pages;

    if (!page_count(1, &pages) || pages != 1) return 0;
    if (!page_count(PAGE_SIZE, &pages) || pages != 1) return 0;
    if (!page_count(PAGE_SIZE + 1, &pages) || pages != 2) return 0;
    if (!page_count(MMAP_MAX_BYTES, &pages) || pages != MMAP_MAX_BYTES / PAGE_SIZE) return 0;
    if (page_count(0, &pages)) return 0;                    // nothing to map
    if (page_count(MMAP_MAX_BYTES + 1, &pages)) return 0;   // too big
    if (page_count(0xFFFFFFFF, &pages)) return 0;           // would wrap to 0 pages
    return 1;
}

static const defence_t defence = {
    "mmap_page_count",
    "limit mmap length and round it to pages without overflow",
    "mmap_length_wrap",
    selftest
};

void def_mmap_page_count_register(void) {
    proc_security_register_defence(&defence);
}

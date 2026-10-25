/*
 * def_null_page.c - DEFENCE null_page: NULL accesses must fault.
 *
 * Stops:  null_deref (security/attack/null_deref)
 * Called: vmm_init() (core/vmm), while building the page tables.
 * Rule:   virtual page 0 (0x0 - 0xFFF) is never mapped.
 * Why:    the first 8 MiB are identity mapped for the kernel. Without
 *         this, page 0 is mapped too, and a NULL pointer access silently
 *         reads or overwrites the BIOS's interrupt table at address 0.
 *         The bug goes unnoticed. With page 0 unmapped, the same bug
 *         stops at once with a page fault that names the address.
 */

#include "def_null_page.h"
#include "registry.h"
#include "vmm.h"

void def_null_page_protect(void) {
    vmm_unmap_page(0);
}

int def_null_page_ok(void) {
    vmm_walk_t w;
    return !vmm_walk(0, &w) && !vmm_walk(NULL_PAGE_END - 1, &w);
}

// The check is on the live page tables, not a pure rule: is page 0 gone?
static int selftest(void) {
    return def_null_page_ok();
}

static const defence_t defence = {
    "null_page",
    "keep page 0 unmapped so NULL pointer accesses fault",
    "null_deref",
    selftest
};

void def_null_page_register(void) {
    registry_add_defence(&defence);
}

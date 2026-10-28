/*
 * def_page_separation.c - DEFENCE page_separation: kernel pages carry no
 *                         user bit, so ring 3 cannot read them.
 *
 * Stops:  meltdown (security/meltdown) - an ILLUSTRATION (see README)
 * Called: the CPU, on every memory access, using the PAGE_USER bit UzOS
 *         sets when it maps memory (core/vmm map, core/umem for ring 3).
 * Rule:   a kernel page is present but has no PAGE_USER bit; a ring 3
 *         read of it raises a page fault. Meltdown's trick is to read
 *         such a byte speculatively and leak it through the cache.
 * Why:    the architectural line "ring 3 may not read kernel memory" is
 *         the U/S bit. UzOS is a simple 32-bit kernel and cannot run a
 *         real Meltdown; the defence shown here is that very line, which
 *         real mitigations (KPTI, microcode) exist to keep unbroken.
 */

#include "modules/cpu_management/security/meltdown/defence/page_separation/def_page_separation.h"
#include "modules/security_management/core/registry/registry.h"
#include "modules/memory_management/core/vmm/vmm.h"   // PAGE_PRESENT, PAGE_USER, vmm_walk

int def_kernel_page_hidden(uint32_t pte) {
    return (pte & PAGE_PRESENT) && !(pte & PAGE_USER);
}

static int selftest(void) {
    if (!def_kernel_page_hidden(PAGE_PRESENT)) return 0;                 // kernel page
    if (def_kernel_page_hidden(PAGE_PRESENT | PAGE_USER)) return 0;      // user page
    if (def_kernel_page_hidden(0)) return 0;                            // not mapped

    // A real kernel page (the kernel base at 1 MB) must be present and
    // NOT user-readable.
    vmm_walk_t w;
    if (vmm_walk(0x00100000, &w) && !def_kernel_page_hidden(w.pte))
        return 0;
    return 1;
}

static const defence_t defence = {
    "page_separation",
    "kernel pages lack the user bit; ring 3 faults if it reads them",
    "meltdown",
    selftest
};

void def_page_separation_register(void) { registry_add_defence(&defence); }

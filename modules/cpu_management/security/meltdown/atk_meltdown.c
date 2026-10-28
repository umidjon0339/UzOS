/*
 * atk_meltdown.c - ATTACK meltdown (ILLUSTRATION): read kernel memory
 *                  from ring 3 through speculation.
 *
 * Breaks:     disclosure - ring 3 must never learn kernel memory
 * Stopped by: page_separation (defence/page_separation)
 *
 * Meltdown reads a kernel byte that the page tables forbid, in the brief
 * window before the CPU raises the fault, and leaks it through a cache
 * side channel. It needs a specific out-of-order CPU; UzOS is a simple
 * 32-bit kernel and CANNOT run a real Meltdown. What we CAN show is the
 * line it attacks: a kernel page has no PAGE_USER bit, so a ring 3 read
 * faults. Keeping that line intact is the whole game (real systems add
 * KPTI and microcode on top).
 *
 * simulate(): explain the speculative read and the cache leak, and why
 *             UzOS cannot reproduce it.
 * launch():   confirm the architectural line holds - a kernel page is
 *             present but not user-readable. 0 findings = line intact.
 * cleanup():  nothing.
 *
 * Uses: page_separation (its defence), ui_printf (ui).
 */

#include "modules/security_management/core/registry/registry.h"
#include "modules/cpu_management/security/meltdown/defence/page_separation/def_page_separation.h"
#include "modules/memory_management/core/vmm/vmm.h"
#include "modules/ui/core/style/ui.h"

static int simulate(void) {
    ui_printf("  Meltdown reads a forbidden kernel byte speculatively,\n");
    ui_printf("  then leaks it through cache timing before the #PF lands\n");
    ui_printf("  it needs an out-of-order CPU; UzOS cannot reproduce it\n");
    ui_printf("  the line it crosses is the PAGE_USER bit on kernel pages\n");
    return 1;
}

static int launch(void) {
    vmm_walk_t w;
    int hidden = vmm_walk(0x00100000, &w) ? def_kernel_page_hidden(w.pte) : 1;
    ui_printf("  kernel page at 1 MB is hidden from ring 3: %s\n",
              hidden ? "yes" : "NO (leaky!)");
    return hidden ? 0 : 1;
}

static int cleanup(void) { return 0; }

static const attack_t attack = {
    "meltdown",
    "read kernel memory from ring 3 via speculation (illustration)",
    INV_DISCLOSURE, SEV_INFO,
    simulate, launch, cleanup,
    "page_separation"
};

void atk_meltdown_register(void) { registry_add_attack(&attack); }

/*
 * atk_null_deref.c - ATTACK null_deref: a NULL pointer bug that goes unseen.
 *
 * Breaks:     consistency - bugs must show up, not corrupt memory quietly
 * Stopped by: null_page (security/defence/null_page)
 *
 * If page 0 is mapped, "p->field" with p == NULL reads or writes real
 * memory at address 0 (the BIOS interrupt table) and nothing complains.
 *
 * simulate(): walk the page tables for a few addresses inside page 0.
 * launch():   the same check on every address a NULL struct access could
 *             reach. A real access is not done: in the kernel it would
 *             (correctly) stop the whole OS with a page fault.
 *
 * Uses: vmm_walk (core/vmm).
 */

#include "security_lab.h"
#include "memory_layout.h"
#include "vmm.h"
#include "vga.h"

static const uint32_t targets[] = {
    0x000,      // *NULL
    0x004,      // NULL->second_field
    0x400,      // BIOS data area, a common "p + offset"
    0xFFC,      // last word of page 0
};
#define TARGET_COUNT (sizeof(targets) / sizeof(targets[0]))

// Print each target and count the ones that are mapped (= bugs would hide).
static int count_mapped(void) {
    int mapped = 0;
    for (unsigned i = 0; i < TARGET_COUNT; i++) {
        vmm_walk_t w;
        int is_mapped = vmm_walk(targets[i], &w);
        mapped += is_mapped;
        vga_print_str("  NULL + ");
        vga_print_hex(targets[i]);
        vga_print_str(is_mapped ? "  MAPPED: the access would succeed silently\n"
                                : "  not mapped: the access page-faults\n");
    }
    return mapped;
}

static int simulate(void) {
    return count_mapped();
}

static int launch(void) {
    int mapped = count_mapped();
    vga_print_str(mapped ? "  NULL bugs would corrupt memory unnoticed\n"
                         : "  every NULL access is caught at once\n");
    return mapped;
}

static const attack_t attack = {
    "null_deref",
    "a NULL pointer access silently uses the memory at address 0",
    INV_CONSISTENCY, SEV_MEDIUM,
    simulate, launch, 0,        // only reads the page tables
    "null_page"
};

void atk_null_deref_register(void) {
    proc_security_register_attack(&attack);
}

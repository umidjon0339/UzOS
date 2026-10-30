/*
 * atk_orphan_process.c - ATTACK orphan_process: let children outlive the
 *                        parent and hope nobody cleans them up.
 *
 * Breaks:     consistency - a dead process must always be freed
 * Stopped by: orphan_reaping (defence/orphan_reaping)
 *
 * When a parent exits before its children, those children are "orphans".
 * On a system that only reaps a zombie when its PARENT calls wait(), an
 * orphan could sit dead forever, holding a process slot - a slow leak.
 * UzOS avoids this with a global reaper that frees any zombie by its
 * state, so a missing parent changes nothing.
 *
 * simulate(): explain orphans and why a parent-only reap would leak them.
 * launch():   confirm the reaper collects a zombie whether or not its
 *             parent is alive. 0 findings = orphans are handled.
 * cleanup():  nothing.
 *
 * Uses: orphan_reaping (its defence), vga.
 */

#include "modules/security_management/core/registry/registry.h"                                            // attack_t, registry_add_attack
#include "modules/process_management/security/orphan_process/defence/orphan_reaping/def_orphan_reaping.h"  // def_orphan_is_collected
#include "modules/device_management/vga/vga.h"                                                             // vga_print_str

static int simulate(void) {
    vga_print_str("  a parent exits first; its children become orphans\n");
    vga_print_str("  reap-only-on-parent-wait would leak them forever\n");
    vga_print_str("  UzOS: a global reaper frees any zombie by its state\n");
    return 1;
}

static int launch(void) {
    int findings = 0;
    if (!def_orphan_is_collected(1, 0)) findings++;   // orphan zombie must be freed
    if (!def_orphan_is_collected(1, 1)) findings++;
    vga_print_str("  orphan zombie collected without its parent: ");
    vga_print_str(def_orphan_is_collected(1, 0) ? "yes\n" : "NO (leak!)\n");
    return findings;
}

static int cleanup(void) { return 0; }

static const attack_t attack = {
    "orphan_process",
    "leave children orphaned and hope nobody reaps them",
    INV_CONSISTENCY, SEV_LOW,
    simulate, launch, cleanup,
    "orphan_reaping"
};

void atk_orphan_process_register(void) { registry_add_attack(&attack); }

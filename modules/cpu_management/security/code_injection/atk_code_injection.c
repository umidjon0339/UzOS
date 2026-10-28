/*
 * atk_code_injection.c - ATTACK code_injection (CONCEPT).
 *
 * Breaks:     integrity - only the program's own code should run
 * Stopped by: nx_pages (defence/nx_pages) - CONCEPT
 *
 * Code injection writes machine code into a data area the program
 * controls (a buffer, the stack) and then tricks the program into
 * jumping to it - so attacker bytes run with the victim's rights. The
 * clean defence is NX / W^X: data pages are non-executable, so the jump
 * faults. UzOS's 32-bit paging has no no-execute bit, so it cannot mark
 * a page NX; this is shown as a CONCEPT and nothing is run.
 *
 * Uses: nx_pages (its defence), ui_printf (ui).
 */

#include "modules/security_management/core/registry/registry.h"
#include "modules/cpu_management/security/code_injection/defence/nx_pages/def_nx_pages.h"
#include "modules/ui/core/style/ui.h"

static int simulate(void) {
    ui_printf("  write machine code into a buffer, then jump into it\n");
    ui_printf("  injected bytes would run with the victim's rights\n");
    ui_printf("  defence: NX/W^X data pages - UzOS has no NX bit\n");
    return 1;
}
static int launch(void) {
    ui_printf("  not run: concept only (no NX bit to enforce it safely)\n");
    return 0;
}
static int cleanup(void) { return 0; }

static const attack_t attack = {
    "code_injection",
    "run bytes written as data by jumping into them (concept)",
    INV_INTEGRITY, SEV_INFO,
    simulate, launch, cleanup,
    "nx_pages"
};
void atk_code_injection_register(void) { registry_add_attack(&attack); }

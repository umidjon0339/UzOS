/*
 * atk_return_oriented_programming.c - ATTACK ROP (CONCEPT).
 *
 * Breaks:     integrity - control flow must follow the program's code
 * Stopped by: wx_and_aslr (defence/wx_and_aslr) - CONCEPT
 *
 * ROP needs no new code. The attacker overflows a stack and lays down a
 * list of RETURN addresses, each pointing at a short tail of existing
 * code (a "gadget") ending in `ret`. The CPU runs gadget after gadget,
 * stitching them into a new program. Building a real chain needs a
 * known binary, a memory bug and no W^X/ASLR. UzOS cannot enforce W^X
 * (32-bit paging has no NX bit), so this is shown as a CONCEPT, not run.
 *
 * Uses: wx_and_aslr (its defence), ui_printf (ui).
 */

#include "modules/security_management/core/registry/registry.h"
#include "modules/cpu_management/security/return_oriented_programming/defence/wx_and_aslr/def_wx_and_aslr.h"
#include "modules/ui/core/style/ui.h"

static int simulate(void) {
    ui_printf("  overflow a stack with a list of return addresses\n");
    ui_printf("  each points at a 'gadget': existing code ending in ret\n");
    ui_printf("  defences: W^X, ASLR, CFI - UzOS has no NX bit to do W^X\n");
    return 1;
}
static int launch(void) {
    ui_printf("  not run: concept only (no NX bit, no safe demo)\n");
    return 0;
}
static int cleanup(void) { return 0; }

static const attack_t attack = {
    "return_oriented_programming",
    "chain existing code gadgets via stacked returns (concept)",
    INV_INTEGRITY, SEV_INFO,
    simulate, launch, cleanup,
    "wx_and_aslr"
};
void atk_return_oriented_programming_register(void) { registry_add_attack(&attack); }

/*
 * atk_rowhammer.c - ATTACK rowhammer (CONCEPT).
 *
 * Breaks:     integrity - memory must hold what was written to it
 * Stopped by: ecc_and_refresh (defence/ecc_and_refresh) - CONCEPT
 *
 * Rowhammer reads one DRAM row very fast, thousands of times, until the
 * electrical disturbance flips a bit in a NEIGHBOURING row - memory the
 * attacker was never allowed to write. It is a physical effect of real
 * DRAM. UzOS runs on emulated memory (QEMU) that does not model charge
 * leakage, so it CANNOT be reproduced and nothing is launched. The real
 * fixes are in the memory controller: ECC, faster refresh, TRR.
 *
 * Uses: ecc_and_refresh (its defence), ui_printf (ui).
 */

#include "modules/security_management/core/registry/registry.h"
#include "modules/cpu_management/security/rowhammer/defence/ecc_and_refresh/def_ecc_and_refresh.h"
#include "modules/ui/core/style/ui.h"

static int simulate(void) {
    ui_printf("  hammer one DRAM row until a neighbour's bit flips\n");
    ui_printf("  it is a physical effect of real DRAM cells\n");
    ui_printf("  emulated memory cannot flip; fixes are ECC + refresh\n");
    return 1;
}
static int launch(void) {
    ui_printf("  nothing to launch: emulated RAM does not leak charge\n");
    return def_memory_not_vulnerable() ? 0 : 1;
}
static int cleanup(void) { return 0; }

static const attack_t attack = {
    "rowhammer",
    "flip a neighbour row's bits by hammering DRAM (concept)",
    INV_INTEGRITY, SEV_INFO,
    simulate, launch, cleanup,
    "ecc_and_refresh"
};
void atk_rowhammer_register(void) { registry_add_attack(&attack); }

/*
 * atk_dhcp_starvation.c - ATTACK dhcp_starvation (CONCEPT): empty the DHCP server's pool (concept)
 *
 * Breaks:     resource - the DHCP server has addresses for every real machine
 * Stopped by: dhcp_snooping (defence/dhcp_snooping) - CONCEPT
 *
 * DISCOVERs from thousands of invented MAC addresses take every address
 * the server has; real machines get none (and often a rogue server waits
 * to serve them). UzOS is a DHCP client, not a server.
 *
 * Nothing is launched; the defence is described, not run.
 *
 * Uses: def_dhcp_snooping_holds (its defence), ui_printf.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/dhcp_starvation/defence/dhcp_snooping/def_dhcp_snooping.h" // def_dhcp_snooping_holds
#include "modules/ui/core/style/ui.h"                                           // ui_printf, ui_color

static int simulate(void) {
    ui_printf("  one DISCOVER per invented MAC: each takes an address from the pool\n");
    ui_printf("  the pool empties; real machines get nothing, a rogue server steps in\n");
    ui_printf("  the defence is on the switch: DHCP snooping, port security\n");
    return 1;
}

static int launch(void) {
    ui_printf("  not run: a concept (see the README)\n");
    return def_dhcp_snooping_holds() ? 0 : 1;
}

static int cleanup(void) {
    return 0;
}

static const attack_t attack = {
    "dhcp_starvation",
    "ask for every address with invented MACs until the pool is empty",
    INV_RESOURCE, SEV_MEDIUM,
    simulate, launch, cleanup,
    "dhcp_snooping"
};

void atk_dhcp_starvation_register(void) { registry_add_attack(&attack); }

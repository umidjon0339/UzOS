/*
 * atk_vlan_hopping.c - ATTACK vlan_hopping (CONCEPT): jump from one VLAN to another (concept)
 *
 * Breaks:     isolation - a VLAN's traffic stays in that VLAN
 * Stopped by: vlan_pruning (defence/vlan_pruning) - CONCEPT
 *
 * With two 802.1Q tags (the outer one the native VLAN) a frame can be
 * forwarded into a VLAN the sender does not belong to; or a host pretends
 * to be a switch (DTP) to get a trunk. UzOS does not handle VLAN tags.
 *
 * Nothing is launched; the defence is described, not run.
 *
 * Uses: def_vlan_pruning_holds (its defence), ui_printf.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/vlan_hopping/defence/vlan_pruning/def_vlan_pruning.h" // def_vlan_pruning_holds
#include "modules/ui/core/style/ui.h"                                           // ui_printf, ui_color

static int simulate(void) {
    ui_printf("  VLANs separate networks on one switch with 802.1Q tags\n");
    ui_printf("  a double-tagged frame loses its outer tag on the trunk and lands in the inner VLAN\n");
    ui_printf("  defences live in the switches: no native VLAN on trunks, DTP off\n");
    return 1;
}

static int launch(void) {
    ui_printf("  not run: a concept (see the README)\n");
    return def_vlan_pruning_holds() ? 0 : 1;
}

static int cleanup(void) {
    return 0;
}

static const attack_t attack = {
    "vlan_hopping",
    "reach another VLAN by double tagging or switch spoofing",
    INV_ISOLATION, SEV_MEDIUM,
    simulate, launch, cleanup,
    "vlan_pruning"
};

void atk_vlan_hopping_register(void) { registry_add_attack(&attack); }

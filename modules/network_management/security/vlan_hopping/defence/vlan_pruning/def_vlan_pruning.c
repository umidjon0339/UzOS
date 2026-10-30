/*
 * def_vlan_pruning.c - DEFENCE vlan_pruning (CONCEPT): trunks carry only the VLANs they need, never the native one untagged.
 *
 * Stops:  vlan_hopping (security/vlan_hopping)
 * Called: nothing in UzOS: switch configuration.
 * Rule:   no untagged native VLAN on trunks; access ports never negotiate trunking.
 * Why:    hopping needs a trunk or a native VLAN to ride on.
 *
 * Uses: nothing in UzOS (see the README).
 */

#include "modules/network_management/security/vlan_hopping/defence/vlan_pruning/def_vlan_pruning.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence

int def_vlan_pruning_holds(void) { return 1; }     // kept outside UzOS: see the README

static int selftest(void) {
    return def_vlan_pruning_holds();
}

static const defence_t defence = {
    "vlan_pruning",
    "trunks carry only the VLANs they need, never the native one untagged",
    "vlan_hopping",
    selftest
};

void def_vlan_pruning_register(void) { registry_add_defence(&defence); }

/*
 * def_dhcp_snooping.c - DEFENCE dhcp_snooping (CONCEPT): the switch limits DHCP requests per port and checks the MAC inside them.
 *
 * Stops:  dhcp_starvation (security/dhcp_starvation)
 * Called: nothing in UzOS: the switch enforces it.
 * Rule:   per-port rate limits; chaddr must equal the frame's source MAC.
 * Why:    the pool is finite; ports are where to ration it.
 *
 * Uses: nothing in UzOS (see the README).
 */

#include "modules/network_management/security/dhcp_starvation/defence/dhcp_snooping/def_dhcp_snooping.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence

int def_dhcp_snooping_holds(void) { return 1; }     // kept outside UzOS: see the README

static int selftest(void) {
    return def_dhcp_snooping_holds();
}

static const defence_t defence = {
    "dhcp_snooping",
    "the switch limits DHCP requests per port and checks the MAC inside them",
    "dhcp_starvation",
    selftest
};

void def_dhcp_snooping_register(void) { registry_add_defence(&defence); }

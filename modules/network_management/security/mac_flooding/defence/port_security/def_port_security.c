/*
 * def_port_security.c - DEFENCE port_security (CONCEPT): a switch port accepts only a few MACs (port security).
 *
 * Stops:  mac_flooding (security/mac_flooding)
 * Called: nothing in UzOS: the switch enforces it.
 * Rule:   a port learns at most N MACs; more shut the port down.
 * Why:    the table is finite; the port is the place to limit it.
 *
 * Uses: nothing in UzOS (see the README).
 */

#include "modules/network_management/security/mac_flooding/defence/port_security/def_port_security.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence

int def_port_security_holds(void) { return 1; }     // kept outside UzOS: see the README

static int selftest(void) {
    return def_port_security_holds();
}

static const defence_t defence = {
    "port_security",
    "a switch port accepts only a few MACs (port security)",
    "mac_flooding",
    selftest
};

void def_port_security_register(void) { registry_add_defence(&defence); }

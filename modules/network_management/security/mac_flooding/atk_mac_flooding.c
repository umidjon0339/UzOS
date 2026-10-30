/*
 * atk_mac_flooding.c - ATTACK mac_flooding (CONCEPT): fill a switch's MAC table (concept)
 *
 * Breaks:     resource - a switch sends each frame only to the port of its MAC
 * Stopped by: port_security (defence/port_security) - CONCEPT
 *
 * A switch learns which MAC is behind which port (the CAM table). Frames
 * from thousands of fake MACs fill it; then it floods every frame to every
 * port, and a sniffer sees everything. UzOS is a host, not a switch.
 *
 * Nothing is launched; the defence is described, not run.
 *
 * Uses: def_port_security_holds (its defence), ui_printf.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/mac_flooding/defence/port_security/def_port_security.h" // def_port_security_holds
#include "modules/ui/core/style/ui.h"                                           // ui_printf, ui_color

static int simulate(void) {
    ui_printf("  a switch keeps MAC -> port in a table of a few thousand entries\n");
    ui_printf("  fake source MACs fill it; then it floods every frame to every port\n");
    ui_printf("  UzOS is a host: the defence lives in the switch (port security)\n");
    return 1;
}

static int launch(void) {
    ui_printf("  not run: a concept (see the README)\n");
    return def_port_security_holds() ? 0 : 1;
}

static int cleanup(void) {
    return 0;
}

static const attack_t attack = {
    "mac_flooding",
    "flood a switch with fake MACs until it acts as a hub",
    INV_RESOURCE, SEV_MEDIUM,
    simulate, launch, cleanup,
    "port_security"
};

void atk_mac_flooding_register(void) { registry_add_attack(&attack); }

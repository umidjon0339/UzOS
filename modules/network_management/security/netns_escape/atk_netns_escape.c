/*
 * atk_netns_escape.c - ATTACK netns_escape (CONCEPT): escape a network namespace (concept)
 *
 * Breaks:     isolation - a container's network stays the container's
 * Stopped by: namespaces (defence/namespaces) - CONCEPT
 *
 * Linux gives each container its own network stack (a namespace): its
 * own interfaces, routes and ports. Bugs in the kernel or a container
 * given CAP_NET_ADMIN on the host let it reach the host's network. UzOS
 * has one network stack for everyone: no namespaces to escape.
 *
 * Nothing is launched; the defence is described, not run.
 *
 * Uses: def_namespaces_holds (its defence), ui_printf.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/netns_escape/defence/namespaces/def_namespaces.h" // def_namespaces_holds
#include "modules/ui/core/style/ui.h"                                           // ui_printf, ui_color

static int simulate(void) {
    ui_printf("  a namespace = its own interfaces, routes, ports, firewall\n");
    ui_printf("  escapes: kernel bugs, CAP_NET_ADMIN leaked, the host's netns shared\n");
    ui_printf("  UzOS has one stack (unshare(CLONE_NEWNET) answers -1)\n");
    return 1;
}

static int launch(void) {
    ui_printf("  not run: a concept (see the README)\n");
    return def_namespaces_holds() ? 0 : 1;
}

static int cleanup(void) {
    return 0;
}

static const attack_t attack = {
    "netns_escape",
    "break out of a network namespace to reach the host's interfaces",
    INV_ISOLATION, SEV_HIGH,
    simulate, launch, cleanup,
    "namespaces"
};

void atk_netns_escape_register(void) { registry_add_attack(&attack); }

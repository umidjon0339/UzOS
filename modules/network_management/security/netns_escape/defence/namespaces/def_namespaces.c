/*
 * def_namespaces.c - DEFENCE namespaces (CONCEPT): each container gets its own network namespace, and never the host's capabilities.
 *
 * Stops:  netns_escape (security/netns_escape)
 * Called: nothing in UzOS: there is one network stack.
 * Rule:   separate namespaces; drop CAP_NET_ADMIN/CAP_NET_RAW in containers.
 * Why:    the stack's tables must not be shared with untrusted code.
 *
 * Uses: nothing in UzOS (see the README).
 */

#include "modules/network_management/security/netns_escape/defence/namespaces/def_namespaces.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence

int def_namespaces_holds(void) { return 1; }     // kept outside UzOS: see the README

static int selftest(void) {
    return def_namespaces_holds();
}

static const defence_t defence = {
    "namespaces",
    "each container gets its own network namespace, and never the host's capabilities",
    "netns_escape",
    selftest
};

void def_namespaces_register(void) { registry_add_defence(&defence); }

/*
 * def_no_accept_redirects.c - DEFENCE no_accept_redirects: redirects are not obeyed.
 *
 * Stops:  icmp_redirect (security/icmp_redirect)
 * Called: icmp_receive() (core/icmp), for every ICMP redirect.
 * Rule:   refuse it (accept_redirects = 0). UzOS's routes come from the
 *         administrator and DHCP only.
 * Why:    a redirect needs no proof; Linux servers and hardened hosts
 *         turn them off.
 *
 * Uses: audit_log (security_management).
 */

#include "modules/network_management/security/icmp_redirect/defence/no_accept_redirects/def_no_accept_redirects.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                       // audit_log

int def_redirect_ok(uint32_t host) {
    (void)host;
    audit_log("no_accept_redirects", "ignored an ICMP redirect (routes change only by hand)");
    return 0;
}

static int selftest(void) {
    return !def_redirect_ok(0x08080808);
}

static const defence_t defence = {
    "no_accept_redirects",
    "ICMP redirects never change the routing table",
    "icmp_redirect",
    selftest
};

void def_no_accept_redirects_register(void) { registry_add_defence(&defence); }

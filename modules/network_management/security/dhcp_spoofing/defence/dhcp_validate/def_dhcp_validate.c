/*
 * def_dhcp_validate.c - DEFENCE dhcp_validate: believe DHCP offers that make sense, from the server we know.
 *
 * Stops:  dhcp_spoofing (security/dhcp_spoofing)
 * Called: on_message() (core/dhcp), for every OFFER.
 * Rule:   the offer names its server; its router lies in the offered
 *         network; and if we had a lease before, it comes from the same
 *         server (a "sticky" server: DHCP snooping lite, on the client).
 * Why:    DHCP has no authentication; switches use DHCP snooping. A
 *         client can at least refuse a stranger once it knows the real one.
 *
 * Uses: audit_log (security_management).
 */

#include "modules/network_management/security/dhcp_spoofing/defence/dhcp_validate/def_dhcp_validate.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                       // audit_log

static int ok(uint32_t ip, uint32_t mask, uint32_t router, uint32_t server, uint32_t known_server) {
    if (!server || !ip)
        return 0;                           // an offer must say who makes it, and what
    if (router && (router & mask) != (ip & mask))
        return 0;                           // a router we could not even reach
    return !known_server || server == known_server;
}

int def_dhcp_offer_ok(uint32_t ip, uint32_t mask, uint32_t router, uint32_t server, uint32_t known_server) {
    if (ok(ip, mask, router, server, known_server))
        return 1;
    audit_log("dhcp_validate", "refused a DHCP offer from an unknown or inconsistent server (rogue?)");
    return 0;
}

static int selftest(void) {
    uint32_t ip = 0x0A00020F, m = 0xFFFFFF00, gw = 0x0A000202;
    if (!ok(ip, m, gw, gw, 0)) return 0;                // the first lease
    if (!ok(ip, m, gw, gw, gw)) return 0;               // the same server again
    if (ok(ip, m, gw, 0x0A000242, gw)) return 0;        // another server
    if (ok(ip, m, 0x01020304, gw, 0)) return 0;         // a router off the network
    if (ok(ip, m, gw, 0, 0)) return 0;                  // no server id
    return 1;
}

static const defence_t defence = {
    "dhcp_validate",
    "an offer must be self-consistent, and once we had a server, from that server",
    "dhcp_spoofing",
    selftest
};

void def_dhcp_validate_register(void) { registry_add_defence(&defence); }

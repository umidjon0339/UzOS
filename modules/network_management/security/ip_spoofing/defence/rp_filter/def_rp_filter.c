/*
 * def_rp_filter.c - DEFENCE rp_filter: strict reverse path filtering (Linux rp_filter=1).
 *
 * Stops:  ip_spoofing (security/ip_spoofing)
 * Called: ipv4_receive() (core/ipv4), for every packet from a real link.
 * Rule:   look the SOURCE address up in the routing table; if the reply
 *         would leave by a different interface than the packet came in
 *         on, the source is forged: drop it.
 * Why:    most spoofed packets claim an address that "lives" elsewhere.
 *
 * Uses: route_lookup (core/route), audit_log (security_management).
 */

#include "modules/network_management/security/ip_spoofing/defence/rp_filter/def_rp_filter.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                       // audit_log
#include "modules/network_management/core/route/route.h"                        // route_lookup

// The rule with the routing table's answer given: the reply's interface.
static int ok(int has_route, const netdev_t *reply_dev, const netdev_t *in_dev) {
    return has_route && reply_dev == in_dev;
}

int def_rp_ok(uint32_t src, netdev_t *dev) {
    route_result_t back;
    int has = route_lookup(src, &back);
    if (ok(has, has ? back.dev : 0, dev))
        return 1;
    audit_log("rp_filter", "dropped a packet with a source that cannot be behind this interface");
    return 0;
}

static int selftest(void) {
    netdev_t a, b;
    if (!ok(1, &a, &a)) return 0;       // replies go back the same way: fine
    if (ok(1, &b, &a)) return 0;        // replies would leave elsewhere: forged
    if (ok(0, 0, &a)) return 0;         // no route back at all
    return 1;
}

static const defence_t defence = {
    "rp_filter",
    "a packet whose source would be routed out another interface is dropped",
    "ip_spoofing",
    selftest
};

void def_rp_filter_register(void) { registry_add_defence(&defence); }

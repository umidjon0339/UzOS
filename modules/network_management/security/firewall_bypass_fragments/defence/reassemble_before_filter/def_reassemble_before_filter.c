/*
 * def_reassemble_before_filter.c - DEFENCE reassemble_before_filter: filter the datagram, not its pieces.
 *
 * Stops:  firewall_bypass_fragments (security/firewall_bypass_fragments)
 * Called: ipv4_receive() (core/ipv4), right before the INPUT chain.
 * Rule:   only a complete datagram (no MF, offset 0) reaches the firewall;
 *         fragments are put together first (and overlaps refused: teardrop).
 * Why:    port and flag rules need the whole transport header (RFC 1858;
 *         Linux's netfilter defragments in conntrack for this).
 *
 * Uses: audit_log (security_management).
 */

#include "modules/network_management/security/firewall_bypass_fragments/defence/reassemble_before_filter/def_reassemble_before_filter.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                       // audit_log

static int whole(uint16_t frag) {
    return (frag & 0x3FFF) == 0;            // no MF, offset 0
}

int def_filter_whole(uint16_t frag) {
    if (whole(frag))
        return 1;
    audit_log("reassemble_before_filter", "a fragment almost reached the firewall: dropped");
    return 0;
}

static int selftest(void) {
    return whole(0) && whole(0x4000) && !whole(0x2000) && !whole(0x0001);
}

static const defence_t defence = {
    "reassemble_before_filter",
    "the firewall sees only whole datagrams: fragments are reassembled first",
    "firewall_bypass_fragments",
    selftest
};

void def_reassemble_before_filter_register(void) { registry_add_defence(&defence); }

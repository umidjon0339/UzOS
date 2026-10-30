/*
 * def_martian_filter.c - DEFENCE martian_filter: no packet from a source that cannot exist.
 *
 * Stops:  land_attack (security/land_attack)
 * Called: ipv4_receive() (core/ipv4), for packets from a real link.
 * Rule:   the source may not be 0.0.0.0 (unless it is a broadcast:
 *         a DHCP client), 255.255.255.255, multicast or class E
 *         (224.0.0.0 and up), nor equal to the destination.
 * Why:    Linux calls these "martians" and drops them (log_martians).
 *
 * Uses: audit_log (security_management).
 */

#include "modules/network_management/security/land_attack/defence/martian_filter/def_martian_filter.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                       // audit_log

static int ok(uint32_t src, uint32_t dst) {
    if (src == 0)
        return dst == 0xFFFFFFFF;           // only a DHCP client's broadcast comes from 0.0.0.0
    if (src == 0xFFFFFFFF || (src >> 28) >= 0xE)
        return 0;                           // broadcast, multicast (224/4), class E (240/4)
    return src != dst;                      // LAND: from the destination itself
}

int def_martian_ok(uint32_t src, uint32_t dst) {
    if (ok(src, dst))
        return 1;
    audit_log("martian_filter", "dropped a martian packet (impossible source)");
    return 0;
}

static int selftest(void) {
    uint32_t a = 0x0A00020F, b = 0x0A000202;
    if (!ok(b, a)) return 0;
    if (ok(a, a)) return 0;                     // LAND
    if (ok(0, a)) return 0;
    if (!ok(0, 0xFFFFFFFF)) return 0;           // DHCP discover
    if (ok(0xFFFFFFFF, a)) return 0;
    if (ok(0xE0000001, a)) return 0;            // multicast as a source
    return 1;
}

static const defence_t defence = {
    "martian_filter",
    "packets from impossible sources (0.0.0.0, broadcast, multicast, ourselves) are dropped",
    "land_attack",
    selftest
};

void def_martian_filter_register(void) { registry_add_defence(&defence); }

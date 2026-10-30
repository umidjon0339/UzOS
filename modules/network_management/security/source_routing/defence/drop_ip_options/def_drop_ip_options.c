/*
 * def_drop_ip_options.c - DEFENCE drop_ip_options: no source routing.
 *
 * Stops:  source_routing (security/source_routing)
 * Called: ipv4_receive() (core/ipv4), when the header has options.
 * Rule:   any LSRR (0x83) or SSRR (0x89) option: drop the packet.
 * Why:    Linux's accept_source_route=0; RFC 7126 recommends dropping.
 *
 * Uses: audit_log (security_management).
 */

#include "modules/network_management/security/source_routing/defence/drop_ip_options/def_drop_ip_options.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                       // audit_log

static int ok(const uint8_t *opt, uint32_t len) {
    uint32_t i = 0;
    while (i < len && opt[i] != 0) {        // 0 = end of the option list
        if (opt[i] == 1) {                  // NOP: one byte
            i++;
            continue;
        }
        if (opt[i] == 0x83 || opt[i] == 0x89)
            return 0;                       // loose / strict source route
        if (i + 1 >= len || opt[i + 1] < 2)
            return 0;                       // a broken option list
        i += opt[i + 1];
    }
    return 1;
}

int def_ip_options_ok(const uint8_t *opt, uint32_t len) {
    if (ok(opt, len))
        return 1;
    audit_log("drop_ip_options", "dropped a packet with a source route option");
    return 0;
}

static int selftest(void) {
    static const uint8_t lsrr[8] = { 0x83, 7, 4, 1, 2, 3, 4, 0 };
    static const uint8_t rr[8] = { 0x07, 7, 4, 0, 0, 0, 0, 0 };
    static const uint8_t broken[4] = { 0x44, 1, 0, 0 };
    return !ok(lsrr, 8) && ok(rr, 8) && !ok(broken, 4);
}

static const defence_t defence = {
    "drop_ip_options",
    "packets with source route options (LSRR, SSRR) are dropped",
    "source_routing",
    selftest
};

void def_drop_ip_options_register(void) { registry_add_defence(&defence); }

/*
 * def_fragment_overlap_check.c - DEFENCE fragment_overlap_check: overlapping fragments drop the datagram.
 *
 * Stops:  teardrop (security/teardrop)
 * Called: ipv4_reassemble() (core/ipv4/fragment.c), before it copies.
 * Rule:   if any 8-byte block a fragment covers is already filled, the
 *         whole datagram is thrown away.
 * Why:    honest senders never overlap; overlaps are teardrop crashes
 *         or firewall evasion.
 *
 * Uses: audit_log (security_management).
 */

#include "modules/network_management/security/teardrop/defence/fragment_overlap_check/def_fragment_overlap_check.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                       // audit_log

static int clear(const uint8_t *filled, uint32_t first, uint32_t last) {
    for (uint32_t b = first; b <= last; b++)
        if (filled[b / 8] & (1 << (b % 8)))
            return 0;
    return 1;
}

int def_frag_no_overlap(const uint8_t *filled, uint32_t first, uint32_t last) {
    if (clear(filled, first, last))
        return 1;
    audit_log("fragment_overlap_check", "overlapping fragments: the datagram was dropped (teardrop?)");
    return 0;
}

static int selftest(void) {
    uint8_t map[4] = { 0x0F, 0, 0, 0 };         // blocks 0-3 filled
    if (!clear(map, 4, 7)) return 0;            // right after: fine
    if (clear(map, 2, 5)) return 0;             // reaches back into 2-3
    if (clear(map, 0, 0)) return 0;             // the same block again
    return 1;
}

static const defence_t defence = {
    "fragment_overlap_check",
    "a datagram whose fragments overlap is dropped whole",
    "teardrop",
    selftest
};

void def_fragment_overlap_check_register(void) { registry_add_defence(&defence); }

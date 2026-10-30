/*
 * def_arp_table_limit.c - DEFENCE arp_table_limit: a cap on neighbours learned from unasked requests.
 *
 * Stops:  arp_flood (security/arp_flood)
 * Called: arp_receive() (core/arp), before a request makes a new entry.
 * Rule:   at most ARP_UNASKED_MAX (8) entries may exist that were
 *         learned from requests (not from answers to our own questions).
 * Why:    learning the asker saves a round trip, but anyone can ask.
 *         Entries we asked for (the gateway, our peers) are never pushed
 *         out by strangers. Linux bounds its table too (gc_thresh).
 *
 * Uses: audit_log (security_management).
 */

#include "modules/network_management/security/arp_flood/defence/arp_table_limit/def_arp_table_limit.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                       // audit_log

static int room(int unasked) {
    return unasked >= 0 && unasked < ARP_UNASKED_MAX;
}

int def_arp_learn_ok(int unasked) {
    if (room(unasked))
        return 1;
    audit_log("arp_table_limit", "ARP: not remembering one more unasked neighbour (flood?)");
    return 0;
}

static int selftest(void) {
    if (!room(0)) return 0;
    if (!room(ARP_UNASKED_MAX - 1)) return 0;
    if (room(ARP_UNASKED_MAX)) return 0;
    if (room(-1)) return 0;
    return 1;
}

static const defence_t defence = {
    "arp_table_limit",
    "at most 8 ARP entries may come from requests we did not ask for",
    "arp_flood",
    selftest
};

void def_arp_table_limit_register(void) { registry_add_defence(&defence); }

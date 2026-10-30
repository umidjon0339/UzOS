/*
 * def_arp_guard.c - DEFENCE arp_guard: ARP may not be told a new MAC for a known address.
 *
 * Stops:  arp_spoofing (security/arp_spoofing)
 * Called: arp_receive() (core/arp), before the sender's MAC is stored.
 * Rule:   - no entry yet: only a request aimed at us may add one (we are
 *           about to answer it anyway); a reply we never asked for adds nothing
 *         - an entry we are asking for (INCOMPLETE): the answer is taken
 *         - a known entry (REACHABLE): only the SAME MAC refreshes it; a
 *           different one is refused and logged
 *         - a hand-made entry (PERMANENT): never changed
 * Why:    ARP has no proof of who is speaking. Refusing changes to a
 *         working entry stops the classic MITM; the entry still expires
 *         after a minute, so a real new card is learned then.
 *
 * Uses: audit_log (security_management), arp_state_t (core/arp).
 */

#include "modules/network_management/security/arp_spoofing/defence/arp_guard/def_arp_guard.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                       // audit_log
#include "modules/network_management/core/arp/arp.h"                            // arp_state_t

static int ok(int state, int same_mac, int request_for_us) {
    switch (state) {
    case ARP_FREE:       return request_for_us;     // it is asking us: we answer it anyway
    case ARP_INCOMPLETE: return 1;                  // the answer to our own question
    case ARP_REACHABLE:  return same_mac;           // a refresh, never a change
    default:             return 0;                  // PERMANENT: set by hand
    }
}

int def_arp_guard_ok(int state, int same_mac, int request_for_us) {
    if (ok(state, same_mac, request_for_us))
        return 1;
    if (state == ARP_REACHABLE || state == ARP_PERMANENT)
        audit_log("arp_guard", "ARP refused: a new MAC for a known address (spoofing?)");
    return 0;
}

static int selftest(void) {
    if (!ok(ARP_INCOMPLETE, 0, 0)) return 0;    // the answer we asked for
    if (!ok(ARP_FREE, 0, 1)) return 0;          // someone asks for us
    if (ok(ARP_FREE, 0, 0)) return 0;           // an unasked reply
    if (!ok(ARP_REACHABLE, 1, 0)) return 0;     // a refresh
    if (ok(ARP_REACHABLE, 0, 0)) return 0;      // a changed MAC: refused
    if (ok(ARP_REACHABLE, 0, 1)) return 0;      // ... even in a request
    if (ok(ARP_PERMANENT, 0, 1)) return 0;      // hand-made entries stay
    return 1;
}

static const defence_t defence = {
    "arp_guard",
    "an ARP packet may not change a known MAC, nor add one nobody asked for",
    "arp_spoofing",
    selftest
};

void def_arp_guard_register(void) { registry_add_defence(&defence); }

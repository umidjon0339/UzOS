/*
 * def_udp_ratelimit.c - DEFENCE udp_ratelimit: a per-source budget for 'port unreachable'.
 *
 * Stops:  udp_flood (security/udp_flood)
 * Called: udp_receive() (core/udp), before it answers a closed port.
 * Rule:   at most UNREACH_PER_SOURCE answers per second to one address
 *         (on top of icmp_ratelimit's global budget).
 * Why:    one flooder must not use up the global budget meant for
 *         everyone, nor get us to answer it at full speed (Linux limits
 *         ICMP per destination: icmp_ratemask, inet_peer).
 *
 * Uses: net_now_ms (core/net_tick), audit_log (security_management).
 */

#include "modules/network_management/security/udp_flood/defence/udp_ratelimit/def_udp_ratelimit.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                       // audit_log
#include "modules/network_management/core/net_tick/net_tick.h"                  // net_now_ms

#define WATCHED 8

static struct {
    uint32_t src, since_ms, sent;
} seen[WATCHED];

static int ok(uint32_t sent) {
    return sent < UNREACH_PER_SOURCE;
}

int def_unreach_ok(uint32_t src) {
    uint32_t now = net_now_ms();
    int slot = 0;
    for (int i = 0; i < WATCHED; i++) {
        if (seen[i].src == src) {
            slot = i;
            break;
        }
        if (seen[i].since_ms < seen[slot].since_ms)
            slot = i;                       // else reuse the oldest
    }
    if (seen[slot].src != src || now - seen[slot].since_ms >= 1000) {
        seen[slot].src = src;
        seen[slot].since_ms = now;
        seen[slot].sent = 0;
    }
    if (ok(seen[slot].sent++))
        return 1;
    if (seen[slot].sent == UNREACH_PER_SOURCE + 1)
        audit_log("udp_ratelimit", "UDP flood: no more 'port unreachable' to this source this second");
    return 0;
}

static int selftest(void) {
    return ok(0) && ok(UNREACH_PER_SOURCE - 1) && !ok(UNREACH_PER_SOURCE);
}

static const defence_t defence = {
    "udp_ratelimit",
    "at most 5 'port unreachable' answers per second to one source",
    "udp_flood",
    selftest
};

void def_udp_ratelimit_register(void) { registry_add_defence(&defence); }

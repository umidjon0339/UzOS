/*
 * def_icmp_ratelimit.c - DEFENCE icmp_ratelimit: ICMP answers are rate-limited.
 *
 * Stops:  icmp_flood (security/icmp_flood)
 * Called: icmp_receive() before an echo reply, icmp_send_error() before
 *         an error (core/icmp).
 * Rule:   a token bucket: ICMP_BURST messages per NET_TICK_MS.
 * Why:    answering is free for the attacker and costly for us; Linux
 *         limits ICMP the same way (icmp_ratelimit, icmp_msgs_per_sec).
 *
 * Uses: net_now_ms (core/net_tick), audit_log (security_management).
 */

#include "modules/network_management/security/icmp_flood/defence/icmp_ratelimit/def_icmp_ratelimit.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                       // audit_log
#include "modules/network_management/core/net_tick/net_tick.h"                  // net_now_ms, NET_TICK_MS

static uint32_t window_start;
static int sent_in_window;

// The rule, with the clock given: 'sent' messages so far in this tick.
static int ok(int sent) {
    return sent < ICMP_BURST;
}

int def_icmp_send_ok(void) {
    uint32_t now = net_now_ms();
    if (now - window_start >= NET_TICK_MS) {
        window_start = now;
        sent_in_window = 0;
    }
    if (ok(sent_in_window)) {
        sent_in_window++;
        return 1;
    }
    if (sent_in_window++ == ICMP_BURST)     // log once per tick, not once per packet
        audit_log("icmp_ratelimit", "ICMP rate limit reached: answers dropped (flood?)");
    return 0;
}

static int selftest(void) {
    return ok(0) && ok(ICMP_BURST - 1) && !ok(ICMP_BURST);
}

static const defence_t defence = {
    "icmp_ratelimit",
    "at most 10 ICMP replies and errors per 100 ms",
    "icmp_flood",
    selftest
};

void def_icmp_ratelimit_register(void) { registry_add_defence(&defence); }

/*
 * def_scan_detect.c - DEFENCE scan_detect: notice a scan and go quiet.
 *
 * Stops:  port_scan (security/port_scan)
 * Called: refuse() (core/tcp), before a RST answers a closed port.
 * Rule:   per source address, count closed-port hits in the current
 *         second; past SCAN_THRESHOLD, stop answering it for
 *         SCAN_QUIET_MS and log it once.
 * Why:    a client does not knock on dozens of closed ports; a quiet
 *         host maps much more slowly (a simple portsentry / psad).
 *
 * Uses: net_now_ms (core/net_tick), audit_log (security_management).
 */

#include "modules/network_management/security/port_scan/defence/scan_detect/def_scan_detect.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                       // audit_log
#include "modules/network_management/core/net_tick/net_tick.h"                  // net_now_ms

#define WATCHED 8

static struct {
    uint32_t src, since_ms, hits, quiet_until;
} seen[WATCHED];

// The rule: 'hits' closed ports already this second.
static int ok(uint32_t hits) {
    return hits < SCAN_THRESHOLD;
}

int def_scan_rst_ok(uint32_t src) {
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
        if (seen[slot].src != src)
            seen[slot].quiet_until = 0;
        seen[slot].src = src;
        seen[slot].since_ms = now;
        seen[slot].hits = 0;
    }
    if ((int32_t)(seen[slot].quiet_until - now) > 0)
        return 0;
    if (ok(seen[slot].hits++))
        return 1;
    seen[slot].quiet_until = now + SCAN_QUIET_MS;
    audit_log("scan_detect", "port scan noticed: no more RSTs to this source for 10 s");
    return 0;
}

static int selftest(void) {
    return ok(0) && ok(SCAN_THRESHOLD - 1) && !ok(SCAN_THRESHOLD);
}

static const defence_t defence = {
    "scan_detect",
    "a source that hits 10 closed ports in a second gets no more RSTs for 10 s",
    "port_scan",
    selftest
};

void def_scan_detect_register(void) { registry_add_defence(&defence); }

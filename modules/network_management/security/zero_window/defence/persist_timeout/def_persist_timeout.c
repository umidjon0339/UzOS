/*
 * def_persist_timeout.c - DEFENCE persist_timeout: give up on a window that never opens.
 *
 * Stops:  zero_window (security/zero_window)
 * Called: retransmit() (core/tcp/tcp_timer.c), before each window probe.
 * Rule:   at most PERSIST_MAX_PROBES probes while the window stays 0;
 *         then the connection ends with ETIMEDOUT.
 * Why:    RFC 1122 says probe forever; RFC 6429 lets a host close a
 *         stalled connection to protect itself (sockstress).
 *
 * Uses: audit_log (security_management).
 */

#include "modules/network_management/security/zero_window/defence/persist_timeout/def_persist_timeout.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                       // audit_log

int def_persist_ok(int probes) {
    if (probes <= PERSIST_MAX_PROBES)
        return 1;
    audit_log("persist_timeout", "the peer's window stayed 0 too long: connection closed");
    return 0;
}

static int selftest(void) {
    return def_persist_ok(1) && def_persist_ok(PERSIST_MAX_PROBES) && !def_persist_ok(PERSIST_MAX_PROBES + 1);
}

static const defence_t defence = {
    "persist_timeout",
    "a window that stays 0 for 4 probes ends the connection",
    "zero_window",
    selftest
};

void def_persist_timeout_register(void) { registry_add_defence(&defence); }

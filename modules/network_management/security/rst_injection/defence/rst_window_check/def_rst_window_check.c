/*
 * def_rst_window_check.c - DEFENCE rst_window_check: RFC 5961: RST must hit rcv_nxt exactly.
 *
 * Stops:  rst_injection (security/rst_injection)
 * Called: segment() (core/tcp), for a RST inside the window.
 * Rule:   seq == rcv_nxt: reset. Anything else: send a challenge ACK
 *         (the real peer, if it really wanted to reset, answers it with
 *         an exact RST).
 * Why:    guessing one exact number of 2^32 is hopeless; guessing a
 *         window was not.
 *
 * Uses: audit_log (security_management).
 */

#include "modules/network_management/security/rst_injection/defence/rst_window_check/def_rst_window_check.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                       // audit_log

int def_rst_exact_ok(uint32_t seq, uint32_t rcv_nxt) {
    if (seq == rcv_nxt)
        return 1;
    audit_log("rst_window_check", "a RST inside the window but not exact: challenge ACK sent");
    return 0;
}

static int selftest(void) {
    return def_rst_exact_ok(1000, 1000) && !def_rst_exact_ok(1001, 1000) && !def_rst_exact_ok(999, 1000);
}

static const defence_t defence = {
    "rst_window_check",
    "a RST resets only at exactly the next expected sequence number",
    "rst_injection",
    selftest
};

void def_rst_window_check_register(void) { registry_add_defence(&defence); }

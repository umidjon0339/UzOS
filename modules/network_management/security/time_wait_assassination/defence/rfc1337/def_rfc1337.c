/*
 * def_rfc1337.c - DEFENCE rfc1337: RFC 1337: ignore RST in TIME_WAIT.
 *
 * Stops:  time_wait_assassination (security/time_wait_assassination)
 * Called: segment() (core/tcp), for a RST to a connection in TIME_WAIT.
 * Rule:   ignore it; TIME_WAIT ends only by its timer.
 * Why:    the hazards RFC 1337 lists (old duplicates in a new
 *         connection) need TIME_WAIT to last. Linux: tcp_rfc1337=1.
 *
 * Uses: audit_log (security_management).
 */

#include "modules/network_management/security/time_wait_assassination/defence/rfc1337/def_rfc1337.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                       // audit_log

int def_timewait_rst_ok(void) {
    audit_log("rfc1337", "ignored a RST in TIME_WAIT (RFC 1337)");
    return 0;
}

static int selftest(void) {
    return !def_timewait_rst_ok();
}

static const defence_t defence = {
    "rfc1337",
    "a RST never ends TIME_WAIT early",
    "time_wait_assassination",
    selftest
};

void def_rfc1337_register(void) { registry_add_defence(&defence); }

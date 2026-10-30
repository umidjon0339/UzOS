/*
 * def_idle_timeout.c - DEFENCE idle_timeout: a deadline for the whole request.
 *
 * Stops:  slowloris (security/slowloris)
 * Called: httpd's serve() (commands/httpd), before every read of a request.
 * Rule:   the request must be complete REQUEST_DEADLINE_MS after the
 *         connection was accepted; then the connection is dropped.
 * Why:    a per-read timeout restarts with every byte; the attacker
 *         sends just enough. Apache's mod_reqtimeout works the same way.
 *
 * Uses: net_now_ms (core/net_tick), audit_log (security_management).
 */

#include "modules/network_management/security/slowloris/defence/idle_timeout/def_idle_timeout.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                       // audit_log
#include "modules/network_management/core/net_tick/net_tick.h"                  // net_now_ms

static int ok(uint32_t elapsed) {
    return elapsed < REQUEST_DEADLINE_MS;
}

int def_request_time_ok(uint32_t started_ms) {
    if (ok(net_now_ms() - started_ms))
        return 1;
    audit_log("idle_timeout", "httpd: dropped a client whose request took too long (slowloris?)");
    return 0;
}

static int selftest(void) {
    return ok(0) && ok(REQUEST_DEADLINE_MS - 1) && !ok(REQUEST_DEADLINE_MS);
}

static const defence_t defence = {
    "idle_timeout",
    "a request must be complete within 5 s in total, not per read",
    "slowloris",
    selftest
};

void def_idle_timeout_register(void) { registry_add_defence(&defence); }

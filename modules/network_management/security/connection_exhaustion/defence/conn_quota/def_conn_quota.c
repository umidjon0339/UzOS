/*
 * def_conn_quota.c - DEFENCE conn_quota: a per-user quota of TCP connections.
 *
 * Stops:  connection_exhaustion (security/connection_exhaustion)
 * Called: tcp_new() (core/tcp), for every new connection.
 * Rule:   at most TCP_USER_MAX for a user without CAP_RESOURCE.
 * Why:    the slots are shared (like process slots: slot_quota).
 *
 * Uses: caps_has (security_management), audit_log (security_management).
 */

#include "modules/network_management/security/connection_exhaustion/defence/conn_quota/def_conn_quota.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                       // audit_log
#include "modules/security_management/core/caps/caps.h"                         // caps_has, CAP_RESOURCE

static int ok(int has_cap, int owned) {
    return has_cap || owned < TCP_USER_MAX;
}

int def_conn_ok(uint32_t uid, int owned) {
    if (ok(caps_has(uid, CAP_RESOURCE), owned))
        return 1;
    audit_log("conn_quota", "TCP connection refused: this user holds its quota");
    return 0;
}

static int selftest(void) {
    return ok(0, 0) && ok(0, TCP_USER_MAX - 1) && !ok(0, TCP_USER_MAX) && ok(1, 100);
}

static const defence_t defence = {
    "conn_quota",
    "a normal user may own at most 8 TCP connections at once",
    "connection_exhaustion",
    selftest
};

void def_conn_quota_register(void) { registry_add_defence(&defence); }

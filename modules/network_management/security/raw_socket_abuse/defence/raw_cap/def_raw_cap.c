/*
 * def_raw_cap.c - DEFENCE raw_cap: raw sockets are root's.
 *
 * Stops:  raw_socket_abuse (security/raw_socket_abuse)
 * Called: socket_create() (core/socket), for SOCK_RAW.
 * Rule:   the caller must hold CAP_NET_RAW.
 * Why:    a raw socket can forge anything the stack would never send.
 *
 * Uses: caps_has (security_management), audit_log (security_management).
 */

#include "modules/network_management/security/raw_socket_abuse/defence/raw_cap/def_raw_cap.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                       // audit_log
#include "modules/security_management/core/caps/caps.h"                         // caps_has

int def_raw_socket_ok(uint32_t uid) {
    if (caps_has(uid, CAP_NET_RAW))
        return 1;
    audit_log("raw_cap", "raw socket refused: needs CAP_NET_RAW");
    return 0;
}

static int selftest(void) {
    return caps_has(0, CAP_NET_RAW) && !caps_has(1, CAP_NET_RAW);
}

static const defence_t defence = {
    "raw_cap",
    "SOCK_RAW needs CAP_NET_RAW",
    "raw_socket_abuse",
    selftest
};

void def_raw_cap_register(void) { registry_add_defence(&defence); }

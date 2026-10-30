/*
 * def_reuseport_owner.c - DEFENCE reuseport_owner: a reuseport group belongs to one user.
 *
 * Stops:  port_hijack (security/port_hijack)
 * Called: udp_bind() (core/udp), when a port is taken by a socket that
 *         has SO_REUSEPORT.
 * Rule:   both sockets set SO_REUSEPORT AND have the same owner.
 * Why:    Linux has the same rule since 3.9 (effective uid must match).
 *
 * Uses: audit_log (security_management).
 */

#include "modules/network_management/security/port_hijack/defence/reuseport_owner/def_reuseport_owner.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                       // audit_log

static int ok(uint32_t uid, int wants, uint32_t owner, int has) {
    return wants && has && uid == owner;
}

int def_reuseport_ok(uint32_t uid, int wants, uint32_t owner, int has) {
    if (ok(uid, wants, owner, has))
        return 1;
    if (wants && has)
        audit_log("reuseport_owner", "SO_REUSEPORT refused: the port belongs to another user");
    return 0;
}

static int selftest(void) {
    return ok(5, 1, 5, 1) && !ok(1, 1, 0, 1) && !ok(5, 0, 5, 1) && !ok(5, 1, 5, 0);
}

static const defence_t defence = {
    "reuseport_owner",
    "SO_REUSEPORT sockets may share a port only with the same user's",
    "port_hijack",
    selftest
};

void def_reuseport_owner_register(void) { registry_add_defence(&defence); }

/*
 * def_port_privilege.c - DEFENCE port_privilege: privileged ports are root's.
 *
 * Stops:  privileged_port_bind (security/privileged_port_bind)
 * Called: the bind of sock_udp.c and sock_tcp.c (core/socket), with the
 *         socket owner's uid.
 * Rule:   1 <= port < 1024 needs CAP_NET_BIND (Linux: CAP_NET_BIND_SERVICE).
 * Why:    clients trust that a low port is the system's service.
 *
 * Uses: caps_has (security_management), audit_log (security_management).
 */

#include "modules/network_management/security/privileged_port_bind/defence/port_privilege/def_port_privilege.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                       // audit_log
#include "modules/security_management/core/caps/caps.h"                         // caps_has

static int ok(int has_cap, uint16_t port) {
    return port == 0 || port >= PORT_PRIVILEGED || has_cap;
}

int def_port_bind_ok(uint32_t uid, uint16_t port) {
    if (ok(caps_has(uid, CAP_NET_BIND), port))
        return 1;
    audit_log("port_privilege", "bind refused: ports below 1024 need CAP_NET_BIND");
    return 0;
}

static int selftest(void) {
    return ok(0, 0) && ok(0, 8080) && !ok(0, 80) && !ok(0, 1023) && ok(1, 80);
}

static const defence_t defence = {
    "port_privilege",
    "binding a port below 1024 needs CAP_NET_BIND",
    "privileged_port_bind",
    selftest
};

void def_port_privilege_register(void) { registry_add_defence(&defence); }

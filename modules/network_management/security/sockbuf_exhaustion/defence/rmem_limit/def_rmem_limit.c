/*
 * def_rmem_limit.c - DEFENCE rmem_limit: charge what a datagram really costs.
 *
 * Stops:  sockbuf_exhaustion (security/sockbuf_exhaustion)
 * Called: socket_queue() (core/socket), before a datagram joins a queue.
 * Rule:   each datagram costs SOCKET_TRUESIZE (its whole packet buffer),
 *         and the charges may not pass the socket's rcvbuf.
 * Why:    Linux charges skb->truesize, not the payload, for the same reason.
 *
 * Uses: audit_log (security_management).
 */

#include "modules/network_management/security/sockbuf_exhaustion/defence/rmem_limit/def_rmem_limit.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                       // audit_log

static int ok(uint32_t charged, uint32_t truesize, uint32_t rcvbuf) {
    return charged <= rcvbuf && truesize <= rcvbuf - charged;
}

int def_rmem_ok(uint32_t charged, uint32_t truesize, uint32_t rcvbuf) {
    if (ok(charged, truesize, rcvbuf))
        return 1;
    audit_log("rmem_limit", "a socket's receive queue is full: datagram dropped");
    return 0;
}

static int selftest(void) {
    return ok(0, 1700, 3400) && ok(1700, 1700, 3400) && !ok(3400, 1700, 3400) && !ok(4000, 1, 3400);
}

static const defence_t defence = {
    "rmem_limit",
    "a socket's queue is charged a whole buffer per datagram, up to its rcvbuf",
    "sockbuf_exhaustion",
    selftest
};

void def_rmem_limit_register(void) { registry_add_defence(&defence); }

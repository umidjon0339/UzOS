/*
 * def_peercred_check.c - DEFENCE peercred_check: the kernel, not the peer, says who connected.
 *
 * Stops:  unix_impersonation (security/unix_impersonation)
 * Called: unix_connect() and unix_pair() (core/unix), when two ends join.
 * Rule:   the credentials stored for the peer come from the connecting
 *         socket's owner (uid, pid) as the kernel knows it, at that
 *         moment; nothing the client sends can change them.
 * Why:    a local server's only trustworthy source of identity.
 *
 * Uses: nothing.
 */

#include "modules/network_management/security/unix_impersonation/defence/peercred_check/def_peercred_check.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence
#include "modules/network_management/core/socket/socket.h"                      // socket_t, socket_cred_t

socket_cred_t def_peer_cred(const socket_t *s) {
    socket_cred_t cred = { s->pid, s->uid, s->uid };    // the owner the kernel set at socket()
    return cred;
}

static int selftest(void) {
    socket_t s;
    s.pid = 7;
    s.uid = 1;
    socket_cred_t c = def_peer_cred(&s);
    return c.pid == 7 && c.uid == 1 && c.gid == 1;
}

static const defence_t defence = {
    "peercred_check",
    "SO_PEERCRED reports the connecting socket's owner, recorded by the kernel at connect()",
    "unix_impersonation",
    selftest
};

void def_peercred_check_register(void) { registry_add_defence(&defence); }

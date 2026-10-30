/*
 * def_icmp_ignore_broadcast.c - DEFENCE icmp_ignore_broadcast: never answer a broadcast ping.
 *
 * Stops:  smurf_attack (security/smurf_attack)
 * Called: icmp_receive() (core/icmp), before an echo reply.
 * Rule:   no reply if the request's destination was a broadcast.
 * Why:    Linux's icmp_echo_ignore_broadcasts=1, the default since Smurf.
 *
 * Uses: audit_log (security_management).
 */

#include "modules/network_management/security/smurf_attack/defence/icmp_ignore_broadcast/def_icmp_ignore_broadcast.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                       // audit_log

int def_echo_reply_ok(int was_broadcast) {
    if (!was_broadcast)
        return 1;
    audit_log("icmp_ignore_broadcast", "ignored a broadcast ping (Smurf?)");
    return 0;
}

static int selftest(void) {
    return def_echo_reply_ok(0) && !def_echo_reply_ok(1);
}

static const defence_t defence = {
    "icmp_ignore_broadcast",
    "an echo request sent to a broadcast address is never answered",
    "smurf_attack",
    selftest
};

void def_icmp_ignore_broadcast_register(void) { registry_add_defence(&defence); }

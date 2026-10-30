/*
 * def_promisc_cap.c - DEFENCE promisc_cap: sniffing the wire is root's privilege.
 *
 * Stops:  promiscuous_sniffing (security/promiscuous_sniffing)
 * Called: netdev_set_promisc() (core/netdev): ip link set promisc,
 *         ifconfig promisc, tcpdump, ioctl(SIOCSIFFLAGS).
 * Rule:   the caller must hold CAP_NET_RAW, to turn it on or off.
 * Why:    promiscuous mode shows frames meant for other machines.
 *
 * Uses: caps_has (security_management), audit_log.
 */

#include "modules/network_management/security/promiscuous_sniffing/defence/promisc_cap/def_promisc_cap.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                       // audit_log
#include "modules/security_management/core/caps/caps.h"                         // caps_has, CAP_NET_RAW

int def_promisc_ok(uint32_t uid) {
    if (caps_has(uid, CAP_NET_RAW))
        return 1;
    audit_log("promisc_cap", "promiscuous mode refused: needs CAP_NET_RAW");
    return 0;
}

static int selftest(void) {
    return caps_has(0, CAP_NET_RAW) && !caps_has(1, CAP_NET_RAW);
}

static const defence_t defence = {
    "promisc_cap",
    "only CAP_NET_RAW may switch an interface to promiscuous mode",
    "promiscuous_sniffing",
    selftest
};

void def_promisc_cap_register(void) { registry_add_defence(&defence); }

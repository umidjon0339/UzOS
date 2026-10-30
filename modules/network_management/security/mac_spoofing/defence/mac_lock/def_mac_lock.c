/*
 * def_mac_lock.c - DEFENCE mac_lock: the MAC address is the administrator's to set.
 *
 * Stops:  mac_spoofing (security/mac_spoofing)
 * Called: netdev_set_mac() (core/netdev): ip link set dev address.
 * Rule:   the caller must hold CAP_NET_ADMIN.
 * Why:    the MAC is the card's identity on the link; whoever sets it
 *         can take another machine's frames and pass MAC filters.
 *
 * Uses: caps_has (security_management), audit_log.
 */

#include "modules/network_management/security/mac_spoofing/defence/mac_lock/def_mac_lock.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                       // audit_log
#include "modules/security_management/core/caps/caps.h"                         // caps_has, CAP_NET_ADMIN

int def_mac_change_ok(uint32_t uid) {
    if (caps_has(uid, CAP_NET_ADMIN))
        return 1;
    audit_log("mac_lock", "MAC change refused: needs CAP_NET_ADMIN");
    return 0;
}

static int selftest(void) {
    return caps_has(0, CAP_NET_ADMIN) && !caps_has(1, CAP_NET_ADMIN);
}

static const defence_t defence = {
    "mac_lock",
    "only CAP_NET_ADMIN may change an interface's MAC address",
    "mac_spoofing",
    selftest
};

void def_mac_lock_register(void) { registry_add_defence(&defence); }

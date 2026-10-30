/*
 * atk_mac_spoofing.c - ATTACK mac_spoofing: change the card's MAC as a normal user
 *
 * Breaks:     privilege - only the administrator decides the card's MAC address
 * Stopped by: mac_lock (defence/mac_lock)
 *
 * Taking another machine's MAC steals its traffic on a switch and
 * slips past MAC-based access lists (a hotel's "paid" list, ...).
 * Only the administrator may change it.
 *
 * simulate(): show eth0's MAC.
 * launch():   as uid 1, netdev_set_mac(eth0, 02:de:ad:00:00:01).
 *             findings = 1 if the MAC changed.
 * cleanup():  put the card's own MAC back.
 *
 * Uses: netdev_set_mac (core/netdev), e1000_mac (core/e1000), ui_*.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/mac_spoofing/defence/mac_lock/def_mac_lock.h" // def_mac_change_ok
#include "modules/network_management/core/netdev/netdev.h"                      // netdev_by_name, netdev_set_mac
#include "modules/network_management/core/e1000/e1000.h"                        // e1000_mac
#include "modules/network_management/core/inet/inet.h"                          // mac_format
#include "modules/ui/core/style/ui.h"                                           // ui_printf, ui_color

#define ATTACKER_UID 1
static const uint8_t STOLEN[6] = { 0x02, 0xde, 0xad, 0x00, 0x00, 0x01 };

static int simulate(void) {
    netdev_t *eth = netdev_by_name("eth0");
    char text[MAC_TEXT];
    if (!eth) {
        ui_printf("  no eth0\n");
        return 0;
    }
    mac_format(eth->mac, text);
    ui_printf("  eth0 is %s; a user who sets another machine's MAC\n", text);
    ui_printf("  receives its frames and passes its MAC-based checks\n");
    return 1;
}

static int launch(void) {
    netdev_t *eth = netdev_by_name("eth0");
    if (!eth) {
        ui_printf("  no eth0\n");
        return 0;
    }
    int r = netdev_set_mac(eth, STOLEN, ATTACKER_UID);
    ui_color(r == 0 ? UI_BAD : UI_GOOD);
    ui_printf("  as uid %d: %s\n", ATTACKER_UID, r == 0 ? "the MAC changed: the user owns the card's identity"
                                                        : "refused (needs CAP_NET_ADMIN)");
    ui_reset();
    return r == 0;
}

static int cleanup(void) {
    netdev_t *eth = netdev_by_name("eth0");
    uint8_t own[6];
    if (!eth || !e1000_present())
        return 0;
    e1000_mac(own);
    return netdev_set_mac(eth, own, 0) == 0;
}

static const attack_t attack = {
    "mac_spoofing",
    "as a normal user, give eth0 another machine's MAC address",
    INV_PRIVILEGE, SEV_MEDIUM,
    simulate, launch, cleanup,
    "mac_lock"
};

void atk_mac_spoofing_register(void) { registry_add_attack(&attack); }

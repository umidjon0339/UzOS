/*
 * atk_promiscuous_sniffing.c - ATTACK promiscuous_sniffing: turn on promiscuous mode as a normal user
 *
 * Breaks:     disclosure - a user sees only the traffic meant for this machine
 * Stopped by: promisc_cap (defence/promisc_cap)
 *
 * Normally the card drops frames for other MACs. In promiscuous mode it
 * keeps them all: on a hub or a mirrored switch port, that is everyone's
 * traffic. tcpdump needs it, so it needs root.
 *
 * simulate(): show eth0's mode.
 * launch():   as uid 1, netdev_set_promisc(eth0, on).
 *             findings = 1 if the card became promiscuous.
 * cleanup():  turn it off again.
 *
 * Uses: netdev_set_promisc (core/netdev), ui_*.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/promiscuous_sniffing/defence/promisc_cap/def_promisc_cap.h" // def_promisc_ok
#include "modules/network_management/core/netdev/netdev.h"                      // netdev_by_name, netdev_set_promisc
#include "modules/ui/core/style/ui.h"                                           // ui_printf, ui_color

#define ATTACKER_UID 1

static int simulate(void) {
    netdev_t *eth = netdev_by_name("eth0");
    if (!eth)
        return 0;
    ui_printf("  eth0 is %s\n", (eth->flags & NETDEV_PROMISC) ? "promiscuous" : "normal: frames for us only");
    ui_printf("  promiscuous, it keeps every frame on the wire: other machines' too\n");
    return 1;
}

static int launch(void) {
    netdev_t *eth = netdev_by_name("eth0");
    if (!eth)
        return 0;
    uint32_t lock = netdev_lock();
    int r = netdev_set_promisc(eth, 1, ATTACKER_UID);
    netdev_unlock(lock);
    ui_color(r == 0 ? UI_BAD : UI_GOOD);
    ui_printf("  as uid %d: %s\n", ATTACKER_UID, r == 0 ? "PROMISCUOUS: the user can read the whole link"
                                                        : "refused (needs CAP_NET_RAW)");
    ui_reset();
    return r == 0;
}

static int cleanup(void) {
    netdev_t *eth = netdev_by_name("eth0");
    if (!eth || !(eth->flags & NETDEV_PROMISC))
        return 0;
    uint32_t lock = netdev_lock();
    netdev_set_promisc(eth, 0, 0);
    netdev_unlock(lock);
    return 1;
}

static const attack_t attack = {
    "promiscuous_sniffing",
    "as a normal user, switch the card to promiscuous mode to sniff the link",
    INV_DISCLOSURE, SEV_HIGH,
    simulate, launch, cleanup,
    "promisc_cap"
};

void atk_promiscuous_sniffing_register(void) { registry_add_attack(&attack); }

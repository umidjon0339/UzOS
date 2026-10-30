/*
 * atk_arp_flood.c - ATTACK arp_flood: fill the ARP cache with fake neighbours
 *
 * Breaks:     resource - the ARP cache has room for the neighbours we really talk to
 * Stopped by: arp_table_limit (defence/arp_table_limit)
 *
 * A request aimed at us makes us remember the asker (we will answer it).
 * Requests from hundreds of invented addresses would fill the 32-entry
 * cache and push out the real neighbours, so every real packet must ask
 * again: slow, and a chance for spoofing.
 *
 * simulate(): count the free entries.
 * launch():   inject 40 requests "who-has <us>, tell 10.0.2.100+i".
 *             findings = fake entries kept beyond the limit (8).
 * cleanup():  delete the fake entries.
 *
 * Uses: netdev_inject (core/netdev), arp_entry / arp_delete (core/arp), ui_*.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/arp_flood/defence/arp_table_limit/def_arp_table_limit.h" // ARP_UNASKED_MAX
#include "modules/network_management/core/netdev/netdev.h"                      // netdev_by_name, netdev_inject, netdev_lock
#include "modules/network_management/core/ethernet/ethernet.h"                  // eth_header_t, ETH_*
#include "modules/network_management/core/arp/arp.h"                            // arp_packet_t, arp_lookup, arp_entry
#include "modules/network_management/core/inet/inet.h"                          // htons, htonl, inet_format, mac_format
#include "lib/string/string.h"                                                  // memcpy, memset
#include "modules/ui/core/style/ui.h"                                           // ui_printf, ui_color

#define FAKES     40
#define FIRST_IP  INET_ADDR(10, 0, 2, 100)

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

static int count_fakes(void) {
    int n = 0;
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        const arp_entry_t *e = arp_entry(i);
        n += e->state != ARP_FREE && e->ip >= FIRST_IP && e->ip < FIRST_IP + FAKES;
    }
    return n;
}

static int simulate(void) {
    int used = 0;
    for (int i = 0; i < ARP_CACHE_SIZE; i++)
        used += arp_entry(i)->state != ARP_FREE;
    ui_printf("  the ARP cache: %d of %d entries used\n", used, ARP_CACHE_SIZE);
    ui_printf("  %d fake requests would take every free one, then push out real neighbours\n",
              ARP_CACHE_SIZE);
    return ARP_CACHE_SIZE - used;
}

static int launch(void) {
    netdev_t *eth = netdev_by_name("eth0");
    if (!eth || !eth->ip) {
        ui_printf("  needs eth0 with an address\n");
        return 0;
    }
    uint8_t frame[ETH_ZLEN], mac[6] = { 0x02, 0, 0, 0, 0, 0 };
    for (int i = 0; i < FAKES; i++) {
        mac[5] = (uint8_t)i;
        int len = build_arp(frame, ETH_BROADCAST, ARP_OP_REQUEST, mac, FIRST_IP + i, ETH_BROADCAST, eth->ip);
        netdev_inject(eth, frame, (uint16_t)len);
    }
    int kept = count_fakes();
    int extra = kept > ARP_UNASKED_MAX ? kept - ARP_UNASKED_MAX : 0;
    ui_printf("  %d fake requests: %d fake neighbours kept (the limit is %d)\n", FAKES, kept, ARP_UNASKED_MAX);
    ui_color(extra ? UI_BAD : UI_GOOD);
    ui_printf("  %s\n", extra ? "the cache is full of fakes: real neighbours were pushed out"
                             : "the rest were answered but not remembered: the cache stays usable");
    ui_reset();
    return extra;
}

static int cleanup(void) {
    int removed = 0;
    uint32_t lock = netdev_lock();
    for (int i = 0; i < FAKES; i++)
        removed += arp_delete(FIRST_IP + i);
    netdev_unlock(lock);
    return removed;
}

static const attack_t attack = {
    "arp_flood",
    "send ARP requests from many fake addresses until the cache is full of them",
    INV_RESOURCE, SEV_MEDIUM,
    simulate, launch, cleanup,
    "arp_table_limit"
};

void atk_arp_flood_register(void) { registry_add_attack(&attack); }

/*
 * atk_udp_flood.c - ATTACK udp_flood: UDP to closed ports, answered one by one
 *
 * Breaks:     resource - datagrams to closed ports do not make us flood their sender
 * Stopped by: udp_ratelimit (defence/udp_ratelimit)
 *
 * Every datagram to a closed port earns an ICMP "port unreachable". A
 * flood (with a forged source) turns us into a reflector aimed at the
 * victim, and fills our own link with answers.
 *
 * launch(): inject 50 datagrams from 10.0.2.89 to closed ports.
 *           findings = 'unreachable' answers beyond the per-source limit.
 *
 * Uses: netdev_inject (core/netdev), icmp_stats (core/icmp), ui_*.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/udp_flood/defence/udp_ratelimit/def_udp_ratelimit.h" // UNREACH_PER_SOURCE
#include "modules/network_management/core/netdev/netdev.h"                      // netdev_by_name, netdev_inject, netdev_lock
#include "modules/network_management/core/ethernet/ethernet.h"                  // eth_header_t, ETH_*
#include "modules/network_management/core/ipv4/ipv4.h"                          // ipv4_header_t, ipv4_stats
#include "modules/network_management/core/icmp/icmp.h"                          // icmp_header_t, icmp_stats
#include "modules/network_management/core/inet/inet.h"                          // htons, inet_checksum, INET_ADDR
#include "lib/string/string.h"                                                  // memcpy, memset
#include "modules/network_management/core/udp/udp.h"                            // udp_header_t, UDP_HLEN
#include "modules/ui/core/style/ui.h"                                           // ui_printf, ui_color

// An IPv4 packet inside an Ethernet frame for 'dev', as a machine on the
// link would send it: 'opt' (opt_len bytes of options), then 'data'.
// Returns the frame's length.
static int build_ip(uint8_t *f, const netdev_t *dev, uint32_t src, uint32_t dst, uint8_t proto,
                    uint16_t id, uint16_t frag, const uint8_t *opt, int opt_len, const void *data, int len) {
    static const uint8_t neighbour[6] = { 0x02, 0x00, 0x00, 0xaa, 0xbb, 0xcc };
    int hlen = IPV4_HLEN + opt_len;
    memset(f, 0, ETH_ZLEN);
    eth_header_t *eth = (eth_header_t *)f;
    memcpy(eth->dest, dev->mac, 6);
    memcpy(eth->src, neighbour, 6);
    eth->type = htons(ETH_P_IP);
    ipv4_header_t *ip = (ipv4_header_t *)(f + ETH_HLEN);
    ip->version_ihl = (uint8_t)(0x40 | (hlen / 4));
    ip->total_length = htons((uint16_t)(hlen + len));
    ip->id = htons(id);
    ip->frag = htons(frag);
    ip->ttl = 64;
    ip->protocol = proto;
    ip->src = htonl(src);
    ip->dst = htonl(dst);
    if (opt_len)
        memcpy(f + ETH_HLEN + IPV4_HLEN, opt, opt_len);
    ip->checksum = inet_checksum(ip, hlen);
    memcpy(f + ETH_HLEN + hlen, data, len);
    return ETH_HLEN + hlen + len < ETH_ZLEN ? ETH_ZLEN : ETH_HLEN + hlen + len;
}

// A UDP header and its data, checksum filled in. Returns the length.
static int build_udp(uint8_t *out, uint32_t src, uint32_t dst, uint16_t sport, uint16_t dport,
                     const void *data, int len) {
    udp_header_t *h = (udp_header_t *)out;
    h->src_port = htons(sport);
    h->dst_port = htons(dport);
    h->length = htons((uint16_t)(UDP_HLEN + len));
    h->checksum = 0;
    memcpy(out + UDP_HLEN, data, len);
    h->checksum = inet_checksum_pseudo(src, dst, IPPROTO_UDP, out, UDP_HLEN + len);
    return UDP_HLEN + len;
}

static int simulate(void) {
    ui_printf("  each datagram to a closed port -> one ICMP 'port unreachable'\n");
    ui_printf("  per source at most %d of them a second\n", UNREACH_PER_SOURCE);
    return 1;
}

static int launch(void) {
    netdev_t *eth = netdev_by_name("eth0");
    if (!eth || !eth->ip) {
        ui_printf("  needs eth0 with an address (dhclient)\n");
        return 0;
    }
    static uint8_t f[ETH_ZLEN + 1600];
    uint8_t data[64];

    uint32_t src = INET_ADDR(10, 0, 2, 89), before = icmp_stats()->out_dest_unreachs;
    for (int i = 0; i < 50; i++) {
        int n = build_udp(data, src, eth->ip, 5000, (uint16_t)(20000 + i), "flood", 5);
        netdev_inject(eth, f, (uint16_t)build_ip(f, eth, src, eth->ip, IPPROTO_UDP, (uint16_t)i, 0, 0, 0, data, n));
    }
    int answers = (int)(icmp_stats()->out_dest_unreachs - before);
    int extra = answers > UNREACH_PER_SOURCE ? answers - UNREACH_PER_SOURCE : 0;
    ui_printf("  50 datagrams to closed ports -> %d 'port unreachable' answers\n", answers);
    ui_color(extra ? UI_BAD : UI_GOOD);
    ui_printf("  %s\n", extra ? "every one was answered: we reflect the flood" : "after a few, the source got no more answers");
    ui_reset();
    return extra;
}

static int cleanup(void) {
    return 0;
}

static const attack_t attack = {
    "udp_flood",
    "a flood of datagrams to closed ports: each would earn an ICMP 'port unreachable'",
    INV_RESOURCE, SEV_MEDIUM,
    simulate, launch, cleanup,
    "udp_ratelimit"
};

void atk_udp_flood_register(void) { registry_add_attack(&attack); }

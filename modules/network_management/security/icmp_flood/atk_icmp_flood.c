/*
 * atk_icmp_flood.c - ATTACK icmp_flood: a ping flood
 *
 * Breaks:     resource - answering pings never takes all of the CPU or the link
 * Stopped by: icmp_ratelimit (defence/icmp_ratelimit)
 *
 * Each echo request costs an echo reply. A flood makes us spend the CPU
 * and the outgoing link on answers (and, with a forged source, we flood
 * someone else).
 *
 * launch(): inject 100 echo requests at once. findings = replies sent
 *           beyond the limit.
 *
 * Uses: netdev_inject (core/netdev), icmp_stats (core/icmp), ui_*.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/icmp_flood/defence/icmp_ratelimit/def_icmp_ratelimit.h" // ICMP_BURST
#include "modules/network_management/core/netdev/netdev.h"                      // netdev_by_name, netdev_inject, netdev_lock
#include "modules/network_management/core/ethernet/ethernet.h"                  // eth_header_t, ETH_*
#include "modules/network_management/core/ipv4/ipv4.h"                          // ipv4_header_t, ipv4_stats
#include "modules/network_management/core/icmp/icmp.h"                          // icmp_header_t, icmp_stats
#include "modules/network_management/core/inet/inet.h"                          // htons, inet_checksum, INET_ADDR
#include "lib/string/string.h"                                                  // memcpy, memset
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

// An ICMP echo request (8 bytes of header + 8 of data), checksum filled in.
static int build_echo(uint8_t *out, uint16_t id, uint16_t seq) {
    icmp_header_t *h = (icmp_header_t *)out;
    memset(out, 0, 16);
    h->type = ICMP_ECHO_REQUEST;
    h->id = htons(id);
    h->seq = htons(seq);
    memcpy(out + ICMP_HLEN, "seclab!!", 8);
    h->checksum = inet_checksum(out, 16);
    return 16;
}

static int simulate(void) {
    ui_printf("  100 echo requests in one burst -> 100 replies, unless limited\n");
    ui_printf("  the limit: %d ICMP messages per %d ms\n", ICMP_BURST, 100);
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

    uint32_t before = icmp_stats()->out_echo_reps;
    for (int i = 0; i < 100; i++)
        netdev_inject(eth, f, (uint16_t)build_ip(f, eth, INET_ADDR(10, 0, 2, 95), eth->ip, IPPROTO_ICMP, (uint16_t)i, 0, 0, 0,
                                                  data, build_echo(data, 5, (uint16_t)i)));
    int replies = (int)(icmp_stats()->out_echo_reps - before);
    int extra = replies > ICMP_BURST ? replies - ICMP_BURST : 0;
    ui_printf("  100 requests -> %d replies (the limit is %d per tick)\n", replies, ICMP_BURST);
    ui_color(extra ? UI_BAD : UI_GOOD);
    ui_printf("  %s\n", extra ? "every request was answered: the flood works" : "the rest were not answered: the flood costs us little");
    ui_reset();
    return extra;
}

static int cleanup(void) {
    return 0;
}

static const attack_t attack = {
    "icmp_flood",
    "a burst of 100 echo requests: every one would be answered",
    INV_RESOURCE, SEV_MEDIUM,
    simulate, launch, cleanup,
    "icmp_ratelimit"
};

void atk_icmp_flood_register(void) { registry_add_attack(&attack); }

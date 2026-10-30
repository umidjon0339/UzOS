/*
 * atk_land_attack.c - ATTACK land_attack: LAND: a packet from ourselves, to ourselves
 *
 * Breaks:     trust - a packet's source is a real, other machine
 * Stopped by: martian_filter (defence/martian_filter)
 *
 * 1997's LAND attack sent a SYN whose source WAS the target (same
 * address, same port). Stacks answered themselves forever and froze.
 * Such "martian" sources (0.0.0.0, broadcast, multicast, the
 * destination itself) never come from a real machine.
 *
 * simulate(): list martian sources.
 * launch():   inject a LAND SYN to port 80 and echo requests from
 *             0.0.0.0 and 255.255.255.255. findings = taken in.
 *
 * Uses: netdev_inject (core/netdev), ipv4_stats (core/ipv4), ui_*.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/land_attack/defence/martian_filter/def_martian_filter.h" // def_martian_ok
#include "modules/network_management/core/netdev/netdev.h"                      // netdev_by_name, netdev_inject, netdev_lock
#include "modules/network_management/core/ethernet/ethernet.h"                  // eth_header_t, ETH_*
#include "modules/network_management/core/ipv4/ipv4.h"                          // ipv4_header_t, ipv4_stats
#include "modules/network_management/core/icmp/icmp.h"                          // icmp_header_t, icmp_stats
#include "modules/network_management/core/inet/inet.h"                          // htons, inet_checksum, INET_ADDR
#include "lib/string/string.h"                                                  // memcpy, memset
#include "modules/network_management/core/tcp/tcp.h"                            // tcp_header_t, TCP_SYN
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

// A bare TCP header (no options, no data) with its checksum.
static int build_tcp(uint8_t *out, uint32_t src, uint32_t dst, uint16_t sport, uint16_t dport,
                     uint32_t seq, uint32_t ack, uint8_t flags) {
    tcp_header_t *h = (tcp_header_t *)out;
    memset(out, 0, TCP_HLEN);
    h->src_port = htons(sport);
    h->dst_port = htons(dport);
    h->seq = htonl(seq);
    h->ack = htonl(ack);
    h->offset = (TCP_HLEN / 4) << 4;
    h->flags = flags;
    h->window = htons(8192);
    h->checksum = inet_checksum_pseudo(src, dst, IPPROTO_TCP, out, TCP_HLEN);
    return TCP_HLEN;
}

static int simulate(void) {
    ui_printf("  martian sources: 0.0.0.0, 255.255.255.255, 224.x-255.x, or\n");
    ui_printf("  the destination itself (LAND: src = dst, sport = dport)\n");
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

    static const char *what[3] = { "LAND SYN us:80 -> us:80", "echo from 0.0.0.0", "echo from 255.255.255.255" };
    int passed = 0;
    for (int i = 0; i < 3; i++) {
        uint32_t before = ipv4_stats()->in_delivers;
        int len;
        if (i == 0)
            len = build_ip(f, eth, eth->ip, eth->ip, IPPROTO_TCP, 2, 0, 0, 0, data,
                           build_tcp(data, eth->ip, eth->ip, 80, 80, 12345, 0, TCP_SYN));
        else
            len = build_ip(f, eth, i == 1 ? 0 : 0xFFFFFFFF, eth->ip, IPPROTO_ICMP, 3, 0, 0, 0, data, build_echo(data, 2, 0));
        netdev_inject(eth, f, (uint16_t)len);
        int in = ipv4_stats()->in_delivers > before;
        ui_printf("  %-28s -> %s\n", what[i], in ? "TAKEN IN" : "dropped");
        passed += in;
    }
    ui_color(passed ? UI_BAD : UI_GOOD);
    ui_printf("  %s\n", passed ? "martian packets reached the protocols" : "every martian was dropped");
    ui_reset();
    return passed;
}

static int cleanup(void) {
    return 0;
}

static const attack_t attack = {
    "land_attack",
    "a SYN from our own address and port to the same address and port (LAND)",
    INV_TRUST, SEV_MEDIUM,
    simulate, launch, cleanup,
    "martian_filter"
};

void atk_land_attack_register(void) { registry_add_attack(&attack); }

/*
 * atk_source_routing.c - ATTACK source_routing: a packet that dictates its own route
 *
 * Breaks:     trust - the network, not the sender, decides a packet's path
 * Stopped by: drop_ip_options (defence/drop_ip_options)
 *
 * The LSRR / SSRR options list the routers a packet must pass. An
 * attacker uses them to reach hosts behind a firewall, or to make
 * replies come back through itself.
 *
 * launch(): inject an echo request carrying LSRR. findings = taken in.
 *
 * Uses: netdev_inject (core/netdev), icmp_stats (core/icmp), ui_*.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/source_routing/defence/drop_ip_options/def_drop_ip_options.h" // def_ip_options_ok
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
    ui_printf("  IP option 0x83 (LSRR): 'pass through 10.0.2.93 on the way'\n");
    ui_printf("  replies would follow the route back: through the attacker\n");
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

    static const uint8_t lsrr[8] = { 0x83, 7, 4, 10, 0, 2, 93, 0 };   // + 1 byte end of options
    uint32_t before = icmp_stats()->in_echos;
    netdev_inject(eth, f, (uint16_t)build_ip(f, eth, INET_ADDR(10, 0, 2, 93), eth->ip, IPPROTO_ICMP, 8, 0,
                                              lsrr, sizeof(lsrr), data, build_echo(data, 7, 0)));
    int in = icmp_stats()->in_echos > before;
    ui_color(in ? UI_BAD : UI_GOOD);
    ui_printf("  %s\n", in ? "the source-routed packet was accepted" : "dropped: source routes are not allowed");
    ui_reset();
    return in;
}

static int cleanup(void) {
    return 0;
}

static const attack_t attack = {
    "source_routing",
    "a packet with a loose source route option (LSRR)",
    INV_TRUST, SEV_MEDIUM,
    simulate, launch, cleanup,
    "drop_ip_options"
};

void atk_source_routing_register(void) { registry_add_attack(&attack); }

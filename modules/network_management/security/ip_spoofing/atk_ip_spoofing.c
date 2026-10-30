/*
 * atk_ip_spoofing.c - ATTACK ip_spoofing: a packet from the wire that says "I am localhost"
 *
 * Breaks:     trust - a packet's source address is where it really came from
 * Stopped by: rp_filter (defence/rp_filter)
 *
 * Services often trust 127.0.0.1 ("only local programs can reach me").
 * A packet from the network that merely CLAIMS that source would be
 * trusted too. The reverse path test asks: would we send a reply to
 * this source out of the interface it came in on? For 127.0.0.1 the
 * answer is lo, not eth0: so it cannot have come from eth0.
 *
 * simulate(): explain the test.
 * launch():   inject echo requests on eth0 from 127.0.0.1 and from our
 *             own address. findings = how many were taken in.
 *
 * Uses: netdev_inject (core/netdev), icmp_stats (core/icmp), ui_*.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/ip_spoofing/defence/rp_filter/def_rp_filter.h" // def_rp_ok
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
    ui_printf("  a packet on eth0 claiming to be from 127.0.0.1 or from us\n");
    ui_printf("  reverse path: a reply to that source would leave by lo, not eth0\n");
    ui_printf("  -> it cannot have come from eth0: it is forged\n");
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

    uint32_t forged[2] = { INET_ADDR(127, 0, 0, 1), eth->ip };
    int passed = 0;
    for (int i = 0; i < 2; i++) {
        uint32_t before = icmp_stats()->in_echos;
        netdev_inject(eth, f, (uint16_t)build_ip(f, eth, forged[i], eth->ip, IPPROTO_ICMP, 1, 0, 0, 0, data, build_echo(data, 1, (uint16_t)i)));
        int in = icmp_stats()->in_echos > before;
        char text[INET_ADDR_TEXT];
        inet_format(forged[i], text);
        ui_printf("  echo from %-15s on eth0 -> %s\n", text, in ? "TAKEN IN" : "dropped");
        passed += in;
    }
    ui_color(passed ? UI_BAD : UI_GOOD);
    ui_printf("  %s\n", passed ? "forged sources were believed" : "both forgeries were dropped at the door");
    ui_reset();
    return passed;
}

static int cleanup(void) {
    return 0;
}

static const attack_t attack = {
    "ip_spoofing",
    "send packets from outside that claim to come from 127.0.0.1 or from ourselves",
    INV_TRUST, SEV_HIGH,
    simulate, launch, cleanup,
    "rp_filter"
};

void atk_ip_spoofing_register(void) { registry_add_attack(&attack); }

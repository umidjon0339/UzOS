/*
 * atk_malformed_ip_header.c - ATTACK malformed_ip_header: IP headers that lie about themselves
 *
 * Breaks:     trust - the IP header's lengths fit the packet that carries them
 * Stopped by: ip_header_validate (defence/ip_header_validate)
 *
 * An IP header says how long it is (IHL) and how long the packet is.
 * A parser that believes "IHL = 3" reads options that are not there; one
 * that believes total length = 4000 reads past the frame.
 *
 * simulate(): list the lies.
 * launch():   inject four broken headers. findings = packets delivered.
 *
 * Uses: netdev_inject (core/netdev), ipv4_stats (core/ipv4), ui_*.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/malformed_ip_header/defence/ip_header_validate/def_ip_header_validate.h" // def_ip_header_ok
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
    ui_printf("  IHL 3 (header shorter than 20 bytes), version 6 in an IPv4 frame,\n");
    ui_printf("  total length 4000 in a 60-byte frame, a wrong checksum\n");
    return 4;
}

static int launch(void) {
    netdev_t *eth = netdev_by_name("eth0");
    if (!eth || !eth->ip) {
        ui_printf("  needs eth0 with an address (dhclient)\n");
        return 0;
    }
    static uint8_t f[ETH_ZLEN + 1600];
    uint8_t data[64];

    static const char *what[4] = { "IHL = 3", "version = 6", "total length 4000", "bad checksum" };
    int passed = 0;
    for (int i = 0; i < 4; i++) {
        int len = build_ip(f, eth, INET_ADDR(10, 0, 2, 99), eth->ip, IPPROTO_ICMP, 4, 0, 0, 0, data, build_echo(data, 3, 0));
        ipv4_header_t *ip = (ipv4_header_t *)(f + ETH_HLEN);
        if (i == 0) ip->version_ihl = 0x43;
        if (i == 1) ip->version_ihl = 0x65;
        if (i == 2) ip->total_length = htons(4000);
        if (i == 3) ip->ttl = 1;                // the checksum no longer matches
        if (i < 3) {
            ip->checksum = 0;
            ip->checksum = inet_checksum(ip, IPV4_HLEN);
        }
        uint32_t before = ipv4_stats()->in_delivers;
        netdev_inject(eth, f, (uint16_t)len);
        int in = ipv4_stats()->in_delivers > before;
        ui_printf("  %-20s -> %s\n", what[i], in ? "DELIVERED" : "dropped");
        passed += in;
    }
    ui_color(passed ? UI_BAD : UI_GOOD);
    ui_printf("  %s\n", passed ? "a broken header was believed" : "every broken header was dropped");
    ui_reset();
    return passed;
}

static int cleanup(void) {
    return 0;
}

static const attack_t attack = {
    "malformed_ip_header",
    "IP headers whose lengths, version or checksum lie",
    INV_TRUST, SEV_MEDIUM,
    simulate, launch, cleanup,
    "ip_header_validate"
};

void atk_malformed_ip_header_register(void) { registry_add_attack(&attack); }

/*
 * atk_teardrop.c - ATTACK teardrop: overlapping fragments
 *
 * Breaks:     integrity - fragments fit together without overlapping
 * Stopped by: fragment_overlap_check (defence/fragment_overlap_check)
 *
 * Teardrop sent a second fragment that started INSIDE the first. Stacks
 * that computed "length = end - start" got a negative number and copied
 * gigabytes. Overlaps also let an attacker change data a firewall
 * already checked. RFC 5722 (IPv6) and Linux simply drop such datagrams.
 *
 * launch(): inject fragment [0, 32) and then [16, 24).
 *           findings = 1 if a datagram was rebuilt from them.
 *
 * Uses: netdev_inject (core/netdev), ipv4_stats (core/ipv4), ui_*.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/teardrop/defence/fragment_overlap_check/def_fragment_overlap_check.h" // def_frag_no_overlap
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

static int simulate(void) {
    ui_printf("  fragment 1: bytes 0-31 (more follow)\n");
    ui_printf("  fragment 2: bytes 16-23 (the last): it lies INSIDE fragment 1\n");
    ui_printf("  'length = 24 - 32' is negative: teardrop crashed Windows and Linux\n");
    return 1;
}

static int launch(void) {
    netdev_t *eth = netdev_by_name("eth0");
    if (!eth || !eth->ip) {
        ui_printf("  needs eth0 with an address (dhclient)\n");
        return 0;
    }
    static uint8_t f[ETH_ZLEN + 1600];

    static uint8_t chunk[32];
    memset(chunk, 'T', sizeof(chunk));
    uint32_t oks = ipv4_stats()->reasm_oks;
    netdev_inject(eth, f, (uint16_t)build_ip(f, eth, INET_ADDR(10, 0, 2, 97), eth->ip, 99, 0x7EA2, IPV4_FLAG_MF, 0, 0, chunk, 32));
    netdev_inject(eth, f, (uint16_t)build_ip(f, eth, INET_ADDR(10, 0, 2, 97), eth->ip, 99, 0x7EA2, 2, 0, 0, chunk, 8));
    int rebuilt = ipv4_stats()->reasm_oks > oks;
    ui_color(rebuilt ? UI_BAD : UI_GOOD);
    ui_printf("  %s\n", rebuilt ? "the overlapping pieces were glued into a datagram" : "the overlap was seen: the datagram was dropped");
    ui_reset();
    return rebuilt;
}

static int cleanup(void) {
    return 0;
}

static const attack_t attack = {
    "teardrop",
    "two fragments that overlap (teardrop, 1997)",
    INV_INTEGRITY, SEV_HIGH,
    simulate, launch, cleanup,
    "fragment_overlap_check"
};

void atk_teardrop_register(void) { registry_add_attack(&attack); }

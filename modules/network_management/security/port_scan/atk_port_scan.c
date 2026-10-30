/*
 * atk_port_scan.c - ATTACK port_scan: scan 40 ports in a burst
 *
 * Breaks:     disclosure - a stranger cannot cheaply map every closed port
 * Stopped by: scan_detect (defence/scan_detect)
 *
 * A scan sends SYNs to many ports; every closed port answers RST, every
 * open one SYN-ACK. One source probing dozens of closed ports per second
 * is not a client: it is a scan.
 *
 * launch(): inject SYNs from 10.0.2.91 to ports 1000-1039.
 *           findings = RSTs we sent beyond the threshold.
 *
 * Uses: netdev_inject (core/netdev), tcp_stats (core/tcp), ui_*.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/port_scan/defence/scan_detect/def_scan_detect.h" // SCAN_THRESHOLD
#include "modules/network_management/core/netdev/netdev.h"                      // netdev_by_name, netdev_inject, netdev_lock
#include "modules/network_management/core/ethernet/ethernet.h"                  // eth_header_t, ETH_*
#include "modules/network_management/core/ipv4/ipv4.h"                          // ipv4_header_t, ipv4_stats
#include "modules/network_management/core/icmp/icmp.h"                          // icmp_header_t, icmp_stats
#include "modules/network_management/core/inet/inet.h"                          // htons, inet_checksum, INET_ADDR
#include "lib/string/string.h"                                                  // memcpy, memset
#include "modules/network_management/core/tcp/tcp.h"                            // tcp_header_t, tcp_conn_t, TCP_*
#include "modules/network_management/core/socket/socket.h"                      // socket_*
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
    ui_printf("  a client touches one or two ports; a scanner touches them all\n");
    ui_printf("  after %d closed ports in a second, a source gets no more RSTs\n", SCAN_THRESHOLD);
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

    uint32_t src = INET_ADDR(10, 0, 2, 91), rsts = tcp_stats()->out_rsts;
    for (int port = 1000; port < 1040; port++) {
        int n = build_tcp(data, src, eth->ip, 41000, (uint16_t)port, 7, 0, TCP_SYN);
        netdev_inject(eth, f, (uint16_t)build_ip(f, eth, src, eth->ip, IPPROTO_TCP, (uint16_t)port, 0, 0, 0, data, n));
    }
    int sent = (int)(tcp_stats()->out_rsts - rsts);
    int extra = sent > SCAN_THRESHOLD ? sent - SCAN_THRESHOLD : 0;
    ui_printf("  40 SYNs to closed ports -> %d RSTs (the threshold is %d)\n", sent, SCAN_THRESHOLD);
    ui_color(extra ? UI_BAD : UI_GOOD);
    ui_printf("  %s\n", extra ? "every port answered: the map is complete" : "the scanner was noticed and gets silence now");
    ui_reset();
    return extra;
}

static int cleanup(void) {
    return 0;
}

static const attack_t attack = {
    "port_scan",
    "SYNs from one address to 40 closed ports within a second",
    INV_DISCLOSURE, SEV_LOW,
    simulate, launch, cleanup,
    "scan_detect"
};

void atk_port_scan_register(void) { registry_add_attack(&attack); }

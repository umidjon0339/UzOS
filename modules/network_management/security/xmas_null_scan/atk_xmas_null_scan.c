/*
 * atk_xmas_null_scan.c - ATTACK xmas_null_scan: NULL / Xmas / SYN-FIN scans
 *
 * Breaks:     disclosure - nonsense segments tell an attacker nothing
 * Stopped by: tcp_flag_sanity (defence/tcp_flag_sanity)
 *
 * Segments with no flags, or with FIN+PSH+URG ("lit like a Christmas
 * tree"), mean nothing; RFC 793 makes a closed port answer them with RST
 * and an open one stay silent. Scanners use that to map ports past
 * simple filters, and to fingerprint the OS.
 *
 * launch(): inject the three to a closed port. findings = RSTs we sent.
 *
 * Uses: netdev_inject (core/netdev), tcp_stats (core/tcp), ui_*.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/xmas_null_scan/defence/tcp_flag_sanity/def_tcp_flag_sanity.h" // def_tcp_flags_ok
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
    ui_printf("  NULL: no flags; Xmas: FIN+PSH+URG; SYN+FIN: open and close at once\n");
    ui_printf("  closed ports answer RST, open ones stay silent: a free port map\n");
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

    static const struct { uint8_t flags; const char *name; } scans[3] = {
        { 0, "NULL" }, { TCP_FIN | TCP_PSH | TCP_URG, "Xmas" }, { TCP_SYN | TCP_FIN, "SYN+FIN" },
    };
    int answered = 0;
    for (int i = 0; i < 3; i++) {
        uint32_t rsts = tcp_stats()->out_rsts;
        uint32_t src = INET_ADDR(10, 0, 2, 92);
        int n = build_tcp(data, src, eth->ip, 40000, 9, 0, 0, scans[i].flags);
        netdev_inject(eth, f, (uint16_t)build_ip(f, eth, src, eth->ip, IPPROTO_TCP, (uint16_t)i, 0, 0, 0, data, n));
        int rst = tcp_stats()->out_rsts > rsts;
        ui_printf("  %-8s to port 9 -> %s\n", scans[i].name, rst ? "RST (port mapped)" : "dropped");
        answered += rst;
    }
    ui_color(answered ? UI_BAD : UI_GOOD);
    ui_printf("  %s\n", answered ? "the scan learns which ports are closed" : "silence: the scan learns nothing");
    ui_reset();
    return answered;
}

static int cleanup(void) {
    return 0;
}

static const attack_t attack = {
    "xmas_null_scan",
    "NULL, Xmas and SYN+FIN segments, whose answers reveal open and closed ports",
    INV_DISCLOSURE, SEV_LOW,
    simulate, launch, cleanup,
    "tcp_flag_sanity"
};

void atk_xmas_null_scan_register(void) { registry_add_attack(&attack); }

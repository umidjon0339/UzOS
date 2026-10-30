/*
 * atk_sack_panic.c - ATTACK sack_panic: announce a tiny MSS
 *
 * Breaks:     resource - the peer cannot make us cut data into absurdly small segments
 * Stopped by: mss_min (defence/mss_min)
 *
 * The peer tells us its biggest segment (MSS). With MSS 8, every 1460
 * bytes become 183 segments: 20x the work, and in Linux 2019 an integer
 * overflow in the retransmit queue ("SACK Panic"). Linux now refuses an
 * MSS below 48.
 *
 * launch(): listen on 7782; inject a SYN with MSS 8 from 10.0.2.90.
 *           findings = 1 if the new connection uses an MSS below 48.
 * cleanup(): close the listener.
 *
 * Uses: socket_* (core/socket), netdev_inject (core/netdev), tcp_conn (core/tcp), ui_*.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/sack_panic/defence/mss_min/def_mss_min.h" // TCP_MSS_MIN
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

#define PORT 7782
static socket_t *listener;

static int simulate(void) {
    ui_printf("  MSS 8: 1460 bytes become 183 segments\n");
    ui_printf("  2019: the retransmit queue overflowed (CVE-2019-11477)\n");
    return 1;
}

static int launch(void) {
    netdev_t *eth = netdev_by_name("eth0");
    if (!eth || !eth->ip) {
        ui_printf("  needs eth0 with an address (dhclient)\n");
        return 0;
    }
    static uint8_t f[ETH_ZLEN + 1600];

    sockaddr_t a;
    uint32_t len = sizeof(a);
    if (socket_create(AF_INET, SOCK_STREAM, 0, &listener) < 0)
        return 0;
    socket_inet_addr(&a, &len, 0, PORT);
    if (socket_bind(listener, &a, len) < 0 || socket_listen(listener, 4) < 0)
        return 0;
    uint32_t src = INET_ADDR(10, 0, 2, 90);
    uint8_t syn[TCP_HLEN + 4];
    build_tcp(syn, 0, 0, 42000, PORT, 5000, 0, TCP_SYN);
    tcp_header_t *h = (tcp_header_t *)syn;
    h->offset = (sizeof(syn) / 4) << 4;
    syn[TCP_HLEN] = 2;                      // option: MSS
    syn[TCP_HLEN + 1] = 4;
    syn[TCP_HLEN + 2] = 0;
    syn[TCP_HLEN + 3] = 8;                  // ... of 8 bytes
    h->checksum = 0;
    h->checksum = inet_checksum_pseudo(src, eth->ip, IPPROTO_TCP, syn, sizeof(syn));
    netdev_inject(eth, f, (uint16_t)build_ip(f, eth, src, eth->ip, IPPROTO_TCP, 1, 0, 0, 0, syn, sizeof(syn)));
    int mss = -1;
    for (int i = 0; i < TCP_MAX_CONNS; i++) {
        const tcp_conn_t *conn = tcp_conn(i);
        if (conn && conn->local_port == PORT && conn->remote_ip == src)
            mss = conn->mss;
    }
    ui_printf("  the peer asked for MSS 8 -> the connection uses %d\n", mss);
    int bad = mss >= 0 && mss < TCP_MSS_MIN;
    ui_color(bad ? UI_BAD : UI_GOOD);
    ui_printf("  %s\n", bad ? "we would cut every write into 8-byte segments" : "the MSS was raised to the minimum");
    ui_reset();
    return bad;
}

static int cleanup(void) {
    if (!listener)
        return 0;
    socket_close(listener);
    listener = 0;
    return 1;
}

static const attack_t attack = {
    "sack_panic",
    "a SYN announcing an MSS of 8 bytes (CVE-2019-11477 \"SACK Panic\")",
    INV_RESOURCE, SEV_HIGH,
    simulate, launch, cleanup,
    "mss_min"
};

void atk_sack_panic_register(void) { registry_add_attack(&attack); }

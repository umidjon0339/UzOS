/*
 * atk_rst_injection.c - ATTACK rst_injection: kill someone else's connection with a forged RST
 *
 * Breaks:     liveness - only the real peer can reset a connection
 * Stopped by: rst_window_check (defence/rst_window_check)
 *
 * RFC 793 accepted a RST anywhere in the receive window: with a 64 KiB
 * window an attacker needs only about 65000 guesses (2004: BGP sessions
 * torn down this way). RFC 5961: only the exact next sequence number
 * resets; anything else in the window gets a "challenge ACK".
 *
 * launch(): make a connection over lo, inject 3 RSTs in the window
 *           (not exact). findings = 1 if the connection died.
 *
 * Uses: socket_* (core/socket), netdev_inject (core/netdev), loopback_dev (core/loopback), ui_*.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/rst_injection/defence/rst_window_check/def_rst_window_check.h" // def_rst_exact_ok
#include "modules/network_management/core/netdev/netdev.h"                      // netdev_by_name, netdev_inject, netdev_lock
#include "modules/network_management/core/ethernet/ethernet.h"                  // eth_header_t, ETH_*
#include "modules/network_management/core/ipv4/ipv4.h"                          // ipv4_header_t, ipv4_stats
#include "modules/network_management/core/icmp/icmp.h"                          // icmp_header_t, icmp_stats
#include "modules/network_management/core/inet/inet.h"                          // htons, inet_checksum, INET_ADDR
#include "lib/string/string.h"                                                  // memcpy, memset
#include "modules/network_management/core/tcp/tcp.h"                            // tcp_header_t, tcp_conn_t, TCP_*
#include "modules/network_management/core/socket/socket.h"                      // socket_*
#include "modules/network_management/core/loopback/loopback.h"                  // loopback_dev
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

// A TCP connection over lo: listener, client, and the accepted server
// end. 0, or -1 if something failed (all closed again).
static int make_pair(uint16_t port, socket_t **l, socket_t **client, socket_t **server) {
    sockaddr_t a;
    uint32_t len = sizeof(a);
    *l = *client = *server = 0;
    if (socket_create(AF_INET, SOCK_STREAM, 0, l) < 0)
        return -1;
    int one = 1;                            // the last run's ends may still hold the port
    socket_setopt(*l, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    socket_inet_addr(&a, &len, 0, port);
    if (socket_bind(*l, &a, len) < 0 || socket_listen(*l, 4) < 0 ||
        socket_create(AF_INET, SOCK_STREAM, 0, client) < 0) {
        socket_close(*l);
        return -1;
    }
    len = sizeof(a);
    socket_inet_addr(&a, &len, INET_ADDR(127, 0, 0, 1), port);
    if (socket_connect(*client, &a, len) < 0 || socket_accept(*l, server) < 0) {
        socket_close(*client);
        socket_close(*l);
        return -1;
    }
    return 0;
}

static void close_pair(socket_t *l, socket_t *client, socket_t *server) {
    if (server) socket_close(server);
    if (client) socket_close(client);
    if (l)      socket_close(l);
}
#define PORT 7779

static int simulate(void) {
    ui_printf("  old rule: a RST anywhere in the window resets (65535 guesses)\n");
    ui_printf("  RFC 5961: only the exact next sequence number; else a challenge ACK\n");
    return 1;
}

static int launch(void) {
    socket_t *l, *c, *s;
    if (make_pair(PORT, &l, &c, &s) < 0)
        return 0;
    netdev_t *lo = loopback_dev();
    tcp_conn_t *victim = s->proto;
    static uint8_t f[ETH_ZLEN + 64];
    uint8_t data[TCP_HLEN];
    static const uint32_t offsets[3] = { 1, 1000, 5000 };
    for (int i = 0; i < 3; i++) {
        uint32_t seq = victim->rcv_nxt + offsets[i];
        int n = build_tcp(data, victim->remote_ip, victim->local_ip, victim->remote_port, victim->local_port, seq, 0, TCP_RST);
        netdev_inject(lo, f, (uint16_t)build_ip(f, lo, victim->remote_ip, victim->local_ip, IPPROTO_TCP, (uint16_t)i, 0, 0, 0, data, n));
    }
    int dead = victim->state == TCP_CLOSED;
    ui_printf("  3 RSTs at rcv_nxt + 1, + 1000, + 5000 -> the connection is %s\n", tcp_state_name(victim->state));
    close_pair(l, c, s);
    ui_color(dead ? UI_BAD : UI_GOOD);
    ui_printf("  %s\n", dead ? "the connection was killed by a guess" : "it lives: each RST got a challenge ACK");
    ui_reset();
    return dead;
}

static int cleanup(void) {
    return 0;
}

static const attack_t attack = {
    "rst_injection",
    "forged RSTs with a sequence number somewhere in the receive window",
    INV_LIVENESS, SEV_MEDIUM,
    simulate, launch, cleanup,
    "rst_window_check"
};

void atk_rst_injection_register(void) { registry_add_attack(&attack); }

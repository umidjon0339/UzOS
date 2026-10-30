/*
 * udp.c - The port table: deliver datagrams to their endpoint, send ours.
 *
 * Which endpoint gets a datagram for port P? The most specific one:
 * one connected to exactly this sender beats one bound to our address,
 * which beats one bound to "any address". If nobody has port P, the
 * sender is told so: ICMP "port unreachable" (that is how a UDP port
 * scan sees a closed port).
 *
 * Uses: packet_* (core/packet), ipv4_send / ipv4_header (core/ipv4),
 *       route_lookup (core/route), icmp_send_error (core/icmp), inet_checksum_pseudo / htons (core/inet),
 *       cpu_irq_save (arch/x86/cpu), memcpy (lib/string).
 */

#include "modules/network_management/core/udp/udp.h"             // this file
#include "modules/network_management/core/ipv4/ipv4.h"           // ipv4_send, ipv4_header
#include "modules/network_management/core/icmp/icmp.h"           // icmp_send_error
#include "modules/network_management/core/route/route.h"         // route_lookup
#include "modules/network_management/core/inet/inet.h"           // inet_checksum_pseudo, htons, NET_E*
#include "arch/x86/cpu/cpu.h"                                     // cpu_irq_save, cpu_irq_restore
#include "lib/string/string.h"                                    // memcpy
#include "modules/network_management/security/udp_flood/defence/udp_ratelimit/def_udp_ratelimit.h" // def_unreach_ok
#include "modules/network_management/security/port_hijack/defence/reuseport_owner/def_reuseport_owner.h" // def_reuseport_ok

static udp_endpoint_t *table[UDP_MAX_ENDPOINTS];
static udp_stats_t stats;
static uint16_t next_ephemeral = UDP_EPHEMERAL_LOW;

udp_stats_t *udp_stats(void) {
    return &stats;
}

const udp_endpoint_t *udp_endpoint(int slot) {
    return slot >= 0 && slot < UDP_MAX_ENDPOINTS ? table[slot] : 0;
}

/* ---------- Binding ---------- */

// Would 'ep' clash on 'port' with an endpoint already bound? Two sockets
// may share a port only through SO_REUSEPORT, and (DEFENCE
// reuseport_owner) only when one user owns both.
static int port_taken(const udp_endpoint_t *ep, uint16_t port) {
    for (int i = 0; i < UDP_MAX_ENDPOINTS; i++) {
        const udp_endpoint_t *o = table[i];
        if (!o || o->local_port != port || (ep->local_ip && o->local_ip && o->local_ip != ep->local_ip))
            continue;
        if (!def_reuseport_ok(ep->uid, ep->reuseport, o->uid, o->reuseport))
            return 1;
    }
    return 0;
}

int udp_bind(udp_endpoint_t *ep) {
    uint32_t flags = cpu_irq_save();
    int result = 0;
    if (!ep->local_port) {
        // Pick the next free ephemeral port.
        int tries = UDP_EPHEMERAL_HIGH - UDP_EPHEMERAL_LOW + 1;
        while (tries-- && port_taken(ep, next_ephemeral))
            next_ephemeral = next_ephemeral >= UDP_EPHEMERAL_HIGH ? UDP_EPHEMERAL_LOW : next_ephemeral + 1;
        if (tries < 0)
            result = -NET_EADDRINUSE;
        else
            ep->local_port = next_ephemeral++;
    } else if (port_taken(ep, ep->local_port)) {
        result = -NET_EADDRINUSE;
    }
    int slot = -1;
    for (int i = 0; i < UDP_MAX_ENDPOINTS && slot < 0; i++)
        if (!table[i])
            slot = i;
    if (!result && slot < 0)
        result = -NET_ENOBUFS;
    if (!result)
        table[slot] = ep;
    cpu_irq_restore(flags);
    return result;
}

void udp_unbind(udp_endpoint_t *ep) {
    uint32_t flags = cpu_irq_save();
    for (int i = 0; i < UDP_MAX_ENDPOINTS; i++)
        if (table[i] == ep)
            table[i] = 0;
    cpu_irq_restore(flags);
}

/* ---------- Sending ---------- */

int udp_send(uint32_t src, uint16_t sport, uint32_t dst, uint16_t dport,
             const void *data, uint16_t len, uint8_t ttl) {
    if (len > PACKET_DATA_MAX - UDP_HLEN - IPV4_HLEN)
        return -NET_EMSGSIZE;
    packet_t *p = packet_alloc();
    if (!p)
        return -NET_ENOBUFS;
    udp_header_t *h = (udp_header_t *)packet_put(p, UDP_HLEN);
    memcpy(packet_put(p, len), data, len);
    h->src_port = htons(sport);
    h->dst_port = htons(dport);
    h->length = htons((uint16_t)(UDP_HLEN + len));
    h->checksum = 0;

    // The checksum covers the IP addresses, so we need to know our source
    // address now. With none given, ask the route, as ipv4_send will.
    if (!src && dst != IPV4_BROADCAST) {
        route_result_t r;
        if (route_lookup(dst, &r))
            src = r.src;
    }
    h->checksum = inet_checksum_pseudo(src, dst, IPPROTO_UDP, p->data, p->len);
    if (h->checksum == 0)
        h->checksum = 0xFFFF;               // 0 means "none"; all ones is the same number
    stats.out_datagrams++;
    return ipv4_send(p, src, dst, IPPROTO_UDP, ttl);
}

/* ---------- Receiving ---------- */

// The best endpoint for a datagram to (dst, dport) from (src, sport).
static udp_endpoint_t *lookup(uint32_t dst, uint16_t dport, uint32_t src, uint16_t sport) {
    udp_endpoint_t *best = 0;
    int best_score = -1;
    for (int i = 0; i < UDP_MAX_ENDPOINTS; i++) {
        udp_endpoint_t *ep = table[i];
        if (!ep || ep->local_port != dport)
            continue;
        if (ep->local_ip && ep->local_ip != dst && dst != IPV4_BROADCAST)
            continue;
        if (ep->remote_ip && (ep->remote_ip != src || ep->remote_port != sport))
            continue;
        int score = (ep->local_ip ? 1 : 0) + (ep->remote_ip ? 2 : 0);
        if (score > best_score) {
            best = ep;
            best_score = score;
        }
    }
    return best;
}

void udp_receive(packet_t *p) {
    ipv4_header_t *ip = ipv4_header(p);
    udp_header_t *h = (udp_header_t *)p->data;
    uint32_t src = ntohl(ip->src), dst = ntohl(ip->dst);

    if (p->len < UDP_HLEN || ntohs(h->length) < UDP_HLEN || ntohs(h->length) > p->len) {
        stats.in_errors++;
        packet_free(p);
        return;
    }
    packet_trim(p, ntohs(h->length));
    if (h->checksum && inet_checksum_pseudo(src, dst, IPPROTO_UDP, p->data, p->len) != 0) {
        stats.in_errors++;                  // damaged
        packet_free(p);
        return;
    }

    uint16_t sport = ntohs(h->src_port), dport = ntohs(h->dst_port);
    udp_endpoint_t *ep = lookup(dst, dport, src, sport);
    if (!ep) {
        stats.no_ports++;                   // closed port: tell the sender ...
        if (dst != IPV4_BROADCAST && def_unreach_ok(src))   // ... DEFENCE udp_ratelimit: not too often
            icmp_send_error(p, ICMP_DEST_UNREACH, ICMP_PORT_UNREACH);
        packet_free(p);
        return;
    }
    stats.in_datagrams++;
    packet_pull(p, UDP_HLEN);
    ep->receive(ep, p, src, sport);
}

void udp_icmp_error(const uint8_t *quote, uint16_t len, int err) {
    const ipv4_header_t *ip = (const ipv4_header_t *)quote;
    uint16_t hlen = (uint16_t)((ip->version_ihl & 0x0F) * 4);
    if (len < hlen + UDP_HLEN)
        return;
    const udp_header_t *h = (const udp_header_t *)(quote + hlen);
    // It quotes OUR datagram: its source port is our endpoint's port.
    udp_endpoint_t *ep = lookup(ntohl(ip->src), ntohs(h->src_port),
                                ntohl(ip->dst), ntohs(h->dst_port));
    if (ep && ep->error)
        ep->error(ep, err);
}

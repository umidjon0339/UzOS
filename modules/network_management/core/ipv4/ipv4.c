/*
 * ipv4.c - Check received IP packets and pass them up; send ours out.
 *
 * Receiving:
 *   1. the header must be sane: version 4, length, checksum, no source route
 *   2. the source must be believable: not a martian, and behind the
 *      interface it came in on (reverse path)
 *   3. it must be for us: UzOS is a host, not a router, so packets for
 *      other addresses are dropped (Linux: ip_forward = 0)
 *   4. fragments wait until the whole datagram is in (fragment.c)
 *   5. the firewall's INPUT chain may drop it (core/firewall)
 *   6. raw sockets get a copy; the protocol number says who reads the
 *      data: ICMP, UDP, TCP
 *
 * Sending:
 *   1. the routing table picks the interface and the next hop
 *   2. we put the header in front (and fragment if it is too long)
 *   3. the firewall's OUTPUT chain may refuse it
 *   4. lo: straight back in; broadcast: to every MAC; else ARP finds the MAC
 *
 * Uses: packet_* (core/packet), route_lookup (core/route), arp_output
 *       (core/arp), ethernet_send (core/ethernet), icmp_receive /
 *       icmp_send_error (core/icmp), udp_receive (core/udp), tcp_receive (core/tcp),
 *       firewall_check (core/firewall), socket_raw_deliver (core/socket), inet_checksum / htons (core/inet),
 *       memcpy (lib/string).
 */

#include "modules/network_management/core/ipv4/ipv4.h"           // this file
#include "modules/network_management/core/route/route.h"         // route_lookup
#include "modules/network_management/core/arp/arp.h"             // arp_output
#include "modules/network_management/core/ethernet/ethernet.h"   // ethernet_send, ETH_BROADCAST
#include "modules/network_management/core/icmp/icmp.h"           // icmp_receive, icmp_send_error
#include "modules/network_management/core/udp/udp.h"             // udp_receive
#include "modules/network_management/core/tcp/tcp.h"             // tcp_receive
#include "modules/network_management/core/firewall/firewall.h"   // firewall_check
#include "modules/network_management/core/socket/socket.h"       // socket_raw_deliver
#include "modules/network_management/core/inet/inet.h"           // inet_checksum, htons, NET_E*
#include "lib/string/string.h"                                    // memcpy
#include "modules/network_management/security/firewall_bypass_fragments/defence/reassemble_before_filter/def_reassemble_before_filter.h"  // def_filter_whole
#include "modules/network_management/security/malformed_ip_header/defence/ip_header_validate/def_ip_header_validate.h" // def_ip_header_ok
#include "modules/network_management/security/source_routing/defence/drop_ip_options/def_drop_ip_options.h" // def_ip_options_ok
#include "modules/network_management/security/land_attack/defence/martian_filter/def_martian_filter.h" // def_martian_ok
#include "modules/network_management/security/ip_spoofing/defence/rp_filter/def_rp_filter.h" // def_rp_ok
#include "modules/network_management/security/ip_id_prediction/defence/random_ip_id/def_random_ip_id.h" // def_next_ip_id
#include "modules/network_management/security/ttl_recon/defence/ttl_normalize/def_ttl_normalize.h" // def_default_ttl

static ipv4_stats_t stats;

ipv4_stats_t *ipv4_stats(void) {
    return &stats;
}

/* ---------- Receiving ---------- */

// Should this host accept a packet for 'dst' that arrived on 'dev'?
static int for_us(netdev_t *dev, uint32_t dst) {
    if (dev->flags & NETDEV_LOOPBACK)
        return 1;                               // lo only carries our own packets
    if (dst == IPV4_BROADCAST)
        return 1;                               // everyone on the link
    if (!dev->ip)
        return 1;                               // no address yet (DHCP): take what comes
    return dst == dev->ip || dst == (dev->ip | ~dev->netmask);   // ours, or our network's broadcast
}

void ipv4_receive(packet_t *p) {
    stats.in_receives++;
    p->net_header = p->data;
    ipv4_header_t *ip = (ipv4_header_t *)p->data;

    // 1. A sane header (DEFENCE ip_header_validate), and no source route
    //    in its options (DEFENCE drop_ip_options)?
    if (!def_ip_header_ok(p->data, p->len)) {
        stats.in_hdr_errors++;
        packet_free(p);
        return;
    }
    uint16_t hlen = (uint16_t)((ip->version_ihl & 0x0F) * 4);
    packet_trim(p, ntohs(ip->total_length));    // Ethernet may have padded a short packet
    if (hlen > IPV4_HLEN && !def_ip_options_ok(p->data + IPV4_HLEN, hlen - IPV4_HLEN)) {
        stats.in_hdr_errors++;
        packet_free(p);
        return;
    }

    // 2. A believable source? Not a martian (DEFENCE martian_filter), and
    //    behind the interface it came in on (DEFENCE rp_filter). lo only
    //    carries our own packets; an interface without an address yet
    //    (DHCP) has no routes to check against.
    uint32_t src = ntohl(ip->src), dst = ntohl(ip->dst);
    if (!(p->dev->flags & NETDEV_LOOPBACK) &&
        (!def_martian_ok(src, dst) || (p->dev->ip && !def_rp_ok(src, p->dev)))) {
        stats.in_addr_errors++;
        packet_free(p);
        return;
    }

    // 3. For us?
    if (!for_us(p->dev, dst)) {
        stats.in_addr_errors++;
        packet_free(p);
        return;
    }

    // 4. A fragment? Wait for the rest.
    if (ntohs(ip->frag) & (IPV4_FLAG_MF | IPV4_OFFSET_MASK)) {
        stats.reasm_reqds++;
        p = ipv4_reassemble(p);
        if (!p)
            return;                             // not complete yet (or thrown away)
        ip = ipv4_header(p);
        hlen = (uint16_t)((ip->version_ihl & 0x0F) * 4);
    }

    // 5. The firewall, after reassembly, so it sees the ports and flags
    //    (DEFENCE reassemble_before_filter: never a fragment).
    if (!def_filter_whole(ntohs(ip->frag)) || !firewall_check(FW_INPUT, p, p->dev)) {
        packet_free(p);
        return;
    }

    // 6. A copy for each raw socket of this protocol, then up to the protocol.
    socket_raw_deliver(p);
    packet_pull(p, hlen);
    p->transport_header = p->data;
    stats.in_delivers++;
    switch (ip->protocol) {
    case IPPROTO_ICMP:
        icmp_receive(p);
        break;
    case IPPROTO_UDP:
        udp_receive(p);
        break;
    case IPPROTO_TCP:
        tcp_receive(p);
        break;
    default:
        stats.in_delivers--;
        stats.in_unknown_protos++;
        icmp_send_error(p, ICMP_DEST_UNREACH, ICMP_PROTO_UNREACH);
        packet_free(p);
    }
}

/* ---------- Sending ---------- */

// The last step: find the MAC for next_hop, frame it, send it.
static int output(netdev_t *dev, uint32_t next_hop, packet_t *p) {
    static const uint8_t no_mac[ETH_ALEN] = { 0 };
    if (!firewall_check(FW_OUTPUT, p, dev)) {
        packet_free(p);
        return -NET_EPERM;                      // as Linux: "Operation not permitted"
    }
    if (dev->flags & NETDEV_LOOPBACK)
        return ethernet_send(dev, no_mac, ETH_P_IP, p) ? 0 : -NET_ENOBUFS;
    if (next_hop == IPV4_BROADCAST || (dev->ip && next_hop == (dev->ip | ~dev->netmask)))
        return ethernet_send(dev, ETH_BROADCAST, ETH_P_IP, p) ? 0 : -NET_ENOBUFS;
    return arp_output(dev, next_hop, p) ? 0 : -NET_EHOSTUNREACH;
}

static void fill_header(ipv4_header_t *ip, uint16_t total, uint16_t id, uint16_t frag,
                        uint8_t ttl, uint8_t protocol, uint32_t src, uint32_t dst) {
    ip->version_ihl = 0x45;                     // version 4, 5 words: no options
    ip->tos = 0;
    ip->total_length = htons(total);
    ip->id = htons(id);
    ip->frag = htons(frag);
    ip->ttl = ttl;
    ip->protocol = protocol;
    ip->checksum = 0;
    ip->src = htonl(src);
    ip->dst = htonl(dst);
    ip->checksum = inet_checksum(ip, IPV4_HLEN);  // over the header only
}

// Too long for the link: cut the data into pieces of (MTU - header),
// rounded down to 8 bytes (the offset counts in 8-byte units).
static int fragment(netdev_t *dev, uint32_t next_hop, packet_t *p, uint16_t id,
                    uint8_t ttl, uint8_t protocol, uint32_t src, uint32_t dst) {
    uint16_t piece = (uint16_t)((dev->mtu - IPV4_HLEN) & ~7u);
    int result = 0;
    for (uint16_t offset = 0; offset < p->len && result == 0; offset += piece) {
        uint16_t len = p->len - offset < piece ? p->len - offset : piece;
        int last = offset + len >= p->len;
        packet_t *f = packet_alloc();
        if (!f) {
            stats.frag_fails++;
            result = -NET_ENOBUFS;
            break;
        }
        memcpy(packet_put(f, len), p->data + offset, len);
        fill_header((ipv4_header_t *)packet_push(f, IPV4_HLEN), IPV4_HLEN + len, id,
                    (uint16_t)((last ? 0 : IPV4_FLAG_MF) | (offset / 8)), ttl, protocol, src, dst);
        stats.frag_creates++;
        result = output(dev, next_hop, f);
    }
    if (result == 0)
        stats.frag_oks++;
    packet_free(p);
    return result;
}

// The interface for a limited broadcast (255.255.255.255): the first
// working one that is not lo. DHCP needs this before we have a route.
static netdev_t *broadcast_dev(void) {
    for (int i = 0; i < netdev_count(); i++) {
        netdev_t *dev = netdev_get(i);
        if ((dev->flags & NETDEV_BROADCAST) && netdev_running(dev))
            return dev;
    }
    return 0;
}

int ipv4_send(packet_t *p, uint32_t src, uint32_t dst, uint8_t protocol, uint8_t ttl) {
    stats.out_requests++;
    route_result_t route;
    if (dst == IPV4_BROADCAST) {
        route.dev = broadcast_dev();
        route.next_hop = IPV4_BROADCAST;
        route.src = route.dev ? route.dev->ip : 0;
        if (!route.dev) {
            stats.out_no_routes++;
            packet_free(p);
            return -NET_ENETUNREACH;
        }
    } else if (!route_lookup(dst, &route)) {
        stats.out_no_routes++;                  // "Network is unreachable"
        packet_free(p);
        return -NET_ENETUNREACH;
    }
    if (!src)
        src = route.src;
    if (!ttl)
        ttl = def_default_ttl();                // DEFENCE ttl_normalize: the common 64
    uint16_t id = def_next_ip_id();             // DEFENCE random_ip_id: never a counter

    if (IPV4_HLEN + p->len > route.dev->mtu)
        return fragment(route.dev, route.next_hop, p, id, ttl, protocol, src, dst);

    ipv4_header_t *ip = (ipv4_header_t *)packet_push(p, IPV4_HLEN);
    if (!ip) {
        packet_free(p);
        return -NET_ENOBUFS;
    }
    fill_header(ip, p->len, id, 0, ttl, protocol, src, dst);
    return output(route.dev, route.next_hop, p);
}

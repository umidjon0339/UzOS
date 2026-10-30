/*
 * icmp.c - Answer pings, report errors, tell ping/traceroute what came back.
 *
 * Errors are never sent about an ICMP error, a broadcast, or a fragment
 * other than the first: otherwise one bad packet could start an
 * endless exchange of errors about errors (RFC 1122).
 *
 * Uses: packet_* (core/packet), ipv4_send / ipv4_header (core/ipv4),
 *       udp_icmp_error (core/udp), tcp_icmp_error (core/tcp),
 *       inet_checksum / htons (core/inet), memcpy (lib/string).
 */

#include "modules/network_management/core/icmp/icmp.h"           // this file
#include "modules/network_management/core/ipv4/ipv4.h"           // ipv4_send, ipv4_header
#include "modules/network_management/core/udp/udp.h"             // udp_icmp_error
#include "modules/network_management/core/tcp/tcp.h"             // tcp_icmp_error
#include "modules/network_management/core/inet/inet.h"           // inet_checksum, htons, ntohs
#include "lib/string/string.h"                                    // memcpy
#include "modules/network_management/core/route/route.h"         // route_add (redirects)
#include "modules/network_management/security/icmp_flood/defence/icmp_ratelimit/def_icmp_ratelimit.h" // def_icmp_send_ok
#include "modules/network_management/security/smurf_attack/defence/icmp_ignore_broadcast/def_icmp_ignore_broadcast.h" // def_echo_reply_ok
#include "modules/network_management/security/icmp_redirect/defence/no_accept_redirects/def_no_accept_redirects.h" // def_redirect_ok

#define ERROR_QUOTE 8               // bytes of the bad packet's data quoted in an error

static icmp_stats_t stats;
static icmp_observer_t observer;

icmp_stats_t *icmp_stats(void) {
    return &stats;
}

void icmp_set_observer(icmp_observer_t fn) {
    observer = fn;
}

/* ---------- Sending ---------- */

// p holds the header + data: fill the checksum and send.
static int send(packet_t *p, uint32_t src, uint32_t dst, uint8_t ttl) {
    icmp_header_t *h = (icmp_header_t *)p->data;
    h->checksum = 0;
    h->checksum = inet_checksum(p->data, p->len);
    stats.out_msgs++;
    return ipv4_send(p, src, dst, IPPROTO_ICMP, ttl);
}

int icmp_send_echo(uint32_t dst, uint16_t id, uint16_t seq, uint8_t ttl,
                   const void *data, uint16_t len) {
    packet_t *p = packet_alloc();
    if (!p)
        return -NET_ENOBUFS;
    icmp_header_t *h = (icmp_header_t *)packet_put(p, ICMP_HLEN);
    uint8_t *payload = packet_put(p, len);
    if (!payload) {
        packet_free(p);
        return -NET_EMSGSIZE;
    }
    h->type = ICMP_ECHO_REQUEST;
    h->code = 0;
    h->id = htons(id);
    h->seq = htons(seq);
    memcpy(payload, data, len);
    stats.out_echos++;
    return send(p, 0, dst, ttl);
}

void icmp_send_error(packet_t *cause, uint8_t type, uint8_t code) {
    ipv4_header_t *ip = (ipv4_header_t *)cause->net_header;
    uint32_t dst = ntohl(ip->dst);
    uint16_t hlen = (uint16_t)((ip->version_ihl & 0x0F) * 4);
    if (dst == IPV4_BROADCAST || (ntohs(ip->frag) & IPV4_OFFSET_MASK))
        return;
    if (ip->protocol == IPPROTO_ICMP) {
        const icmp_header_t *inner = (const icmp_header_t *)(cause->net_header + hlen);
        if (inner->type != ICMP_ECHO_REQUEST && inner->type != ICMP_ECHO_REPLY)
            return;                             // never an error about an error
    }
    if (!def_icmp_send_ok())                    // DEFENCE icmp_ratelimit
        return;

    packet_t *p = packet_alloc();
    if (!p)
        return;
    // Quote the IP header and the first 8 bytes after it: enough for the
    // sender to see which of its packets (and ports) it was.
    uint16_t data_len = ntohs(ip->total_length) - hlen;
    uint16_t quote = hlen + (data_len < ERROR_QUOTE ? data_len : ERROR_QUOTE);
    icmp_header_t *h = (icmp_header_t *)packet_put(p, ICMP_HLEN);
    h->type = type;
    h->code = code;
    h->id = 0;
    h->seq = 0;
    memcpy(packet_put(p, quote), cause->net_header, quote);
    if (type == ICMP_DEST_UNREACH)
        stats.out_dest_unreachs++;
    send(p, dst, ntohl(ip->src), 0);
}

/* ---------- Receiving ---------- */

// An error quotes our packet: if it was an echo request, which one?
static void quoted_echo(const uint8_t *quote, uint16_t len, icmp_event_t *e) {
    const ipv4_header_t *ip = (const ipv4_header_t *)quote;
    if (len < IPV4_HLEN)
        return;
    uint16_t hlen = (uint16_t)((ip->version_ihl & 0x0F) * 4);
    if (ip->protocol != IPPROTO_ICMP || len < hlen + ICMP_HLEN)
        return;
    const icmp_header_t *inner = (const icmp_header_t *)(quote + hlen);
    e->id = ntohs(inner->id);
    e->seq = ntohs(inner->seq);
}

void icmp_receive(packet_t *p) {
    stats.in_msgs++;
    ipv4_header_t *ip = ipv4_header(p);
    icmp_header_t *h = (icmp_header_t *)p->data;
    if (p->len < ICMP_HLEN || inet_checksum(p->data, p->len) != 0) {
        stats.in_errors++;
        packet_free(p);
        return;
    }

    icmp_event_t e = { h->type, h->code, ntohl(ip->src), ip->ttl, 0, 0, 0, 0 };
    switch (h->type) {
    case ICMP_ECHO_REQUEST: {
        stats.in_echos++;
        // Answer with the same id, seq and data: turn this very packet
        // around (the new IP header is pushed over the old one).
        uint32_t to = ntohl(ip->src), asked = ntohl(ip->dst);
        int broadcast = asked == IPV4_BROADCAST ||
                        (p->dev->ip && asked == (p->dev->ip | ~p->dev->netmask));
        // DEFENCE icmp_ignore_broadcast (Smurf) and icmp_ratelimit (floods).
        if (!def_echo_reply_ok(broadcast) || !def_icmp_send_ok()) {
            packet_free(p);
            return;
        }
        h->type = ICMP_ECHO_REPLY;
        stats.out_echo_reps++;
        send(p, broadcast ? 0 : asked, to, 0);  // 0: answer from our own address
        return;
    }
    case ICMP_ECHO_REPLY:
        stats.in_echo_reps++;
        e.id = ntohs(h->id);
        e.seq = ntohs(h->seq);
        e.data = p->data + ICMP_HLEN;
        e.data_len = p->len - ICMP_HLEN;
        break;
    case ICMP_REDIRECT:
        // "Use another router for this host": the new router is in the
        // id/seq field, the host is the destination of the quoted packet.
        // DEFENCE no_accept_redirects: never obeyed.
        if (p->len >= ICMP_HLEN + IPV4_HLEN) {
            uint32_t host = ntohl(((const ipv4_header_t *)(p->data + ICMP_HLEN))->dst);
            uint32_t gateway;
            memcpy(&gateway, &h->id, 4);        // 4 bytes across id and seq
            gateway = ntohl(gateway);
            if (def_redirect_ok(host))
                route_add(host, 0xFFFFFFFF, gateway, p->dev, 0, ROUTE_STATIC);
        }
        break;
    case ICMP_DEST_UNREACH:
    case ICMP_TIME_EXCEEDED:
        if (h->type == ICMP_DEST_UNREACH)
            stats.in_dest_unreachs++;
        else
            stats.in_time_excds++;
        quoted_echo(p->data + ICMP_HLEN, p->len - ICMP_HLEN, &e);
        // About a UDP datagram or TCP segment of ours? Tell its socket.
        if (h->type == ICMP_DEST_UNREACH && p->len >= ICMP_HLEN + IPV4_HLEN) {
            uint8_t proto = ((ipv4_header_t *)(p->data + ICMP_HLEN))->protocol;
            int err = h->code == ICMP_PORT_UNREACH || h->code == ICMP_PROTO_UNREACH
                      ? -NET_ECONNREFUSED : -NET_EHOSTUNREACH;
            if (proto == IPPROTO_UDP)
                udp_icmp_error(p->data + ICMP_HLEN, p->len - ICMP_HLEN, err);
            else if (proto == IPPROTO_TCP)
                tcp_icmp_error(p->data + ICMP_HLEN, p->len - ICMP_HLEN, err);
        }
        break;
    }
    if (observer)
        observer(&e);
    packet_free(p);
}

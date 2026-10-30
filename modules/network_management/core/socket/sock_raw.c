/*
 * sock_raw.c - SOCK_RAW: whole IP packets, for one protocol number.
 *
 *   int fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);    needs CAP_NET_RAW
 *
 *   receive  a COPY of every incoming packet of that protocol, with its
 *            IP header in front (the normal path still gets it too)
 *   send     the bytes after the IP header (ICMP header and data, ...);
 *            the kernel puts the IP header in front
 *
 * This is how ping, traceroute and nmap's SYN scan work on Linux: they
 * build the protocol header themselves. It is also why a raw socket is
 * root's only: it can forge any packet (security/raw_socket_abuse).
 *
 * Uses: packet_* (core/packet), ipv4_send / ipv4_header_t (core/ipv4),
 *       socket_queue / socket_inet_* (socket.c), NET_E* (core/inet), memcpy.
 */

#include "modules/network_management/core/socket/socket.h"       // socket_t, socket_ops_t, socket_*
#include "modules/network_management/core/ipv4/ipv4.h"          // ipv4_send, ipv4_header_t
#include "modules/network_management/core/inet/inet.h"          // NET_E*, ntohl
#include "lib/string/string.h"                                  // memcpy

static int raw_connect(socket_t *s, const sockaddr_t *addr, uint32_t len) {
    uint32_t ip;
    uint16_t port;
    int r = socket_inet_parse(addr, len, &ip, &port);
    if (r < 0)
        return r;
    s->remote_ip = ip;                          // a default destination for send()
    s->state = SS_CONNECTED;
    return 0;
}

static int raw_send(socket_t *s, const void *buf, uint32_t len, int flags,
                    const sockaddr_t *addr, uint32_t addrlen) {
    (void)flags;
    uint32_t ip = s->remote_ip;
    uint16_t port;
    if (addr) {
        int r = socket_inet_parse(addr, addrlen, &ip, &port);
        if (r < 0)
            return r;
    } else if (s->state != SS_CONNECTED) {
        return -NET_EDESTADDRREQ;
    }
    if (len > PACKET_DATA_MAX - IPV4_HLEN)
        return -NET_EMSGSIZE;
    packet_t *p = packet_from(buf, (uint16_t)len);
    if (!p)
        return -NET_ENOBUFS;
    int r = ipv4_send(p, 0, ip, (uint8_t)s->protocol, s->ttl);
    return r < 0 ? r : (int)len;
}

static int raw_recv(socket_t *s, void *buf, uint32_t len, int flags,
                    sockaddr_t *addr, uint32_t *addrlen) {
    packet_t *p = (flags & MSG_PEEK) ? packet_queue_peek(&s->rx) : packet_queue_pop(&s->rx);
    if (!p)
        return -NET_EAGAIN;
    uint32_t n = p->len < len ? p->len : len;
    memcpy(buf, p->data, n);                    // the IP header first, as Linux
    if (addr && addrlen)
        socket_inet_addr(addr, addrlen, ntohl(((const ipv4_header_t *)p->data)->src), 0);
    if (!(flags & MSG_PEEK)) {
        s->rx_charged -= SOCKET_TRUESIZE;
        packet_free(p);
    }
    return (int)n;
}

static int raw_name(socket_t *s, int peer, sockaddr_t *addr, uint32_t *len) {
    socket_inet_addr(addr, len, peer ? s->remote_ip : 0, 0);
    return 0;
}

static int raw_poll(socket_t *s) {
    return POLLOUT | (s->rx.count ? POLLIN : 0);
}

const socket_ops_t socket_raw_ops = {
    .connect = raw_connect,
    .send    = raw_send,
    .recv    = raw_recv,
    .name    = raw_name,
    .poll    = raw_poll,
};

/*
 * sock_udp.c - SOCK_DGRAM sockets on top of UDP.
 *
 *   bind     take a port in UDP's table (0 = any free one)
 *   connect  only talk to one peer: send() needs no address, and
 *            datagrams from anyone else are not delivered
 *   sendto   one datagram; a socket that never called bind gets a free
 *            port now ("autobind")
 *   recvfrom one datagram from the queue; a longer one than 'len' is cut
 *            (the rest is lost, as in Linux)
 *
 * Datagrams arrive in the card's interrupt (on_datagram) and wait in
 * the socket's queue, charged against its rcvbuf.
 *
 * Uses: udp_bind / udp_unbind / udp_send (core/udp), route_* (core/route),
 *       socket_queue / socket_wake / socket_inet_* (socket.c), ntohs /
 *       ntohl (core/inet), memcpy (lib/string).
 */

#include "modules/network_management/core/socket/socket.h"       // socket_t, socket_ops_t, socket_*
#include "modules/network_management/core/udp/udp.h"            // udp_bind, udp_send
#include "modules/network_management/core/route/route.h"        // route_lookup, route_is_local
#include "modules/network_management/core/ipv4/ipv4.h"          // ipv4_header_t, IPV4_BROADCAST
#include "modules/network_management/core/inet/inet.h"          // NET_E*, ntohs, ntohl
#include "lib/string/string.h"                                  // memcpy
#include "modules/network_management/security/privileged_port_bind/defence/port_privilege/def_port_privilege.h"  // def_port_bind_ok

#define UDP_MAX_PAYLOAD (PACKET_DATA_MAX - IPV4_HLEN - UDP_HLEN)

/* ---------- From UDP (in the interrupt) ---------- */

static void on_datagram(udp_endpoint_t *ep, packet_t *p, uint32_t src, uint16_t sport) {
    (void)src;
    (void)sport;                                // still in the packet's headers
    socket_queue((socket_t *)ep->owner, p);
}

static void on_error(udp_endpoint_t *ep, int err) {
    socket_t *s = ep->owner;
    if (s->remote_ip) {                         // Linux reports it to connected sockets only
        s->error = -err;
        socket_wake(s);
    }
}

/* ---------- Binding ---------- */

static int do_bind(socket_t *s, uint32_t ip, uint16_t port) {
    if (s->udp.receive)
        return -NET_EINVAL;                     // bound already
    if (ip && !route_is_local(ip))
        return -NET_EADDRNOTAVAIL;              // not one of our addresses
    if (!def_port_bind_ok(s->uid, port))
        return -NET_EACCES;                     // DEFENCE port_privilege: below 1024 is root's
    s->udp.local_ip = ip;
    s->udp.reuseport = s->reuseport;
    s->udp.local_port = port;
    s->udp.uid = s->uid;
    s->udp.receive = on_datagram;
    s->udp.error = on_error;
    s->udp.owner = s;
    int r = udp_bind(&s->udp);
    if (r < 0) {
        s->udp.receive = 0;
        return r;
    }
    s->local_ip = ip;
    s->local_port = s->udp.local_port;          // the chosen one, if port was 0
    return 0;
}

static int udp_sock_bind(socket_t *s, const sockaddr_t *addr, uint32_t len) {
    uint32_t ip;
    uint16_t port;
    int r = socket_inet_parse(addr, len, &ip, &port);
    return r < 0 ? r : do_bind(s, ip, port);
}

static int udp_sock_connect(socket_t *s, const sockaddr_t *addr, uint32_t len) {
    if (addr && len >= 2 && addr->family == AF_UNSPEC) {
        s->remote_ip = s->udp.remote_ip = 0;    // "disconnect"
        s->remote_port = s->udp.remote_port = 0;
        s->state = SS_UNCONNECTED;
        return 0;
    }
    uint32_t ip;
    uint16_t port;
    int r = socket_inet_parse(addr, len, &ip, &port);
    if (r < 0)
        return r;
    route_result_t route;
    if (ip != IPV4_BROADCAST && !route_lookup(ip, &route))
        return -NET_ENETUNREACH;
    if (!s->udp.receive && (r = do_bind(s, 0, 0)) < 0)
        return r;
    s->remote_ip = s->udp.remote_ip = ip;
    s->remote_port = s->udp.remote_port = port;
    if (!s->local_ip && ip != IPV4_BROADCAST)
        s->local_ip = route.src;                // getsockname now shows the address used
    s->state = SS_CONNECTED;
    return 0;
}

/* ---------- Data ---------- */

static int udp_sock_send(socket_t *s, const void *buf, uint32_t len, int flags,
                         const sockaddr_t *addr, uint32_t addrlen) {
    (void)flags;
    uint32_t ip;
    uint16_t port;
    if (addr) {
        int r = socket_inet_parse(addr, addrlen, &ip, &port);
        if (r < 0)
            return r;
    } else if (s->state == SS_CONNECTED) {
        ip = s->remote_ip;
        port = s->remote_port;
    } else {
        return -NET_EDESTADDRREQ;               // sendto() needs an address
    }
    if (len > UDP_MAX_PAYLOAD)
        return -NET_EMSGSIZE;
    if (ip == IPV4_BROADCAST && !s->broadcast)
        return -NET_EACCES;                     // broadcasting must be asked for (SO_BROADCAST)
    if (!s->udp.receive) {
        int r = do_bind(s, 0, 0);               // autobind: a free port
        if (r < 0)
            return r;
    }
    int r = udp_send(s->udp.local_ip, s->local_port, ip, port, buf, (uint16_t)len, s->ttl);
    return r < 0 ? r : (int)len;
}

static int udp_sock_recv(socket_t *s, void *buf, uint32_t len, int flags,
                         sockaddr_t *addr, uint32_t *addrlen) {
    if (s->error && s->rx.count == 0) {
        int err = s->error;
        s->error = 0;
        return -err;                            // e.g. ECONNREFUSED from ICMP
    }
    packet_t *p = (flags & MSG_PEEK) ? packet_queue_peek(&s->rx) : packet_queue_pop(&s->rx);
    if (!p)
        return -NET_EAGAIN;
    uint32_t n = p->len < len ? p->len : len;
    memcpy(buf, p->data, n);
    if (addr && addrlen) {
        const ipv4_header_t *ip = (const ipv4_header_t *)p->net_header;
        const udp_header_t *udp = (const udp_header_t *)p->transport_header;
        socket_inet_addr(addr, addrlen, ntohl(ip->src), ntohs(udp->src_port));
    }
    if (!(flags & MSG_PEEK)) {
        s->rx_charged -= SOCKET_TRUESIZE;
        packet_free(p);
    }
    return (int)n;
}

/* ---------- The rest ---------- */

static int udp_sock_name(socket_t *s, int peer, sockaddr_t *addr, uint32_t *len) {
    if (peer && s->state != SS_CONNECTED)
        return -NET_ENOTCONN;
    socket_inet_addr(addr, len, peer ? s->remote_ip : s->local_ip,
                     peer ? s->remote_port : s->local_port);
    return 0;
}

static int udp_sock_poll(socket_t *s) {
    int bits = POLLOUT;                         // UDP can always send (or drop)
    if (s->rx.count || s->shut_rd)
        bits |= POLLIN;
    return bits;
}

static void udp_sock_close(socket_t *s) {
    if (s->udp.receive)
        udp_unbind(&s->udp);
}

const socket_ops_t socket_udp_ops = {
    .bind     = udp_sock_bind,
    .connect  = udp_sock_connect,
    .send     = udp_sock_send,
    .recv     = udp_sock_recv,
    .name     = udp_sock_name,
    .poll     = udp_sock_poll,
    .close    = udp_sock_close,
};

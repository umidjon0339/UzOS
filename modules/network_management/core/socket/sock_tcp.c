/*
 * sock_tcp.c - SOCK_STREAM sockets on top of TCP connections.
 *
 *   server:  socket -> bind(port) -> listen -> accept -> recv / send -> close
 *   client:  socket -> connect(addr) -> send / recv -> close
 *
 * The socket keeps a tcp_conn_t (socket.proto), made at the first bind,
 * listen or connect. accept() makes a NEW socket for each connection
 * the listener collected. Closing the socket hands the connection back
 * to TCP, which finishes it (FIN, TIME_WAIT) on its own.
 *
 * Uses: tcp_* (core/tcp), route_is_local (core/route), socket_create /
 *       socket_inet_* (socket.c), NET_E* (core/inet).
 */

#include "modules/network_management/core/socket/socket.h"       // socket_t, socket_ops_t, socket_*
#include "modules/network_management/core/tcp/tcp.h"            // tcp_*
#include "modules/network_management/core/route/route.h"        // route_is_local
#include "modules/network_management/core/inet/inet.h"          // NET_E*
#include "modules/network_management/security/privileged_port_bind/defence/port_privilege/def_port_privilege.h"  // def_port_bind_ok

// The socket's connection, made the first time it is needed.
static tcp_conn_t *conn_of(socket_t *s) {
    if (!s->proto) {
        tcp_conn_t *c = tcp_new(s->uid);
        if (c)
            c->sock = s;
        s->proto = c;
    }
    return s->proto;
}

static int tcp_sock_bind(socket_t *s, const sockaddr_t *addr, uint32_t len) {
    uint32_t ip;
    uint16_t port;
    int r = socket_inet_parse(addr, len, &ip, &port);
    if (r < 0)
        return r;
    if (ip && !route_is_local(ip))
        return -NET_EADDRNOTAVAIL;
    if (!def_port_bind_ok(s->uid, port))
        return -NET_EACCES;                     // DEFENCE port_privilege: below 1024 is root's
    tcp_conn_t *c = conn_of(s);
    if (!c)
        return -NET_ENOBUFS;
    if (c->local_port)
        return -NET_EINVAL;                     // bound already
    r = tcp_bind(c, ip, port, s->reuseaddr);
    if (r == 0) {
        s->local_ip = c->local_ip;
        s->local_port = c->local_port;
    }
    return r;
}

static int tcp_sock_listen(socket_t *s, int backlog) {
    tcp_conn_t *c = conn_of(s);
    if (!c)
        return -NET_ENOBUFS;
    int r = tcp_listen(c, backlog);
    if (r == 0) {
        s->state = SS_LISTENING;
        s->local_port = c->local_port;
    }
    return r;
}

static int tcp_sock_accept(socket_t *s, socket_t **child) {
    tcp_conn_t *l = s->proto;
    if (!l || s->state != SS_LISTENING)
        return -NET_EINVAL;
    tcp_conn_t *c = tcp_accept(l);
    if (!c)
        return -NET_EAGAIN;                     // nobody waiting yet

    socket_t *ns;
    int r = socket_create(AF_INET, SOCK_STREAM, 0, &ns);
    if (r < 0) {
        tcp_close(c);                           // no socket for it: abort politely
        return r;
    }
    ns->proto = c;
    c->sock = ns;
    ns->state = SS_CONNECTED;
    ns->local_ip = c->local_ip;
    ns->local_port = c->local_port;
    ns->remote_ip = c->remote_ip;
    ns->remote_port = c->remote_port;
    *child = ns;
    return 0;
}

static int tcp_sock_connect(socket_t *s, const sockaddr_t *addr, uint32_t len) {
    uint32_t ip;
    uint16_t port;
    int r = socket_inet_parse(addr, len, &ip, &port);
    if (r < 0)
        return r;
    tcp_conn_t *c = conn_of(s);
    if (!c)
        return -NET_ENOBUFS;
    r = tcp_connect(c, ip, port);
    if (r == -NET_EINPROGRESS) {
        s->state = SS_CONNECTING;
        s->local_ip = c->local_ip;
        s->local_port = c->local_port;
        s->remote_ip = ip;
        s->remote_port = port;
    }
    return r;
}

static int tcp_sock_send(socket_t *s, const void *buf, uint32_t len, int flags,
                         const sockaddr_t *addr, uint32_t addrlen) {
    (void)flags;
    (void)addr;                                 // a stream has its peer already
    (void)addrlen;
    tcp_conn_t *c = s->proto;
    if (!c || s->state == SS_LISTENING)
        return -NET_ENOTCONN;
    return tcp_send(c, buf, len);
}

static int tcp_sock_recv(socket_t *s, void *buf, uint32_t len, int flags,
                         sockaddr_t *addr, uint32_t *addrlen) {
    tcp_conn_t *c = s->proto;
    if (!c || s->state == SS_LISTENING)
        return -NET_ENOTCONN;
    int r = tcp_recv(c, buf, len, (flags & MSG_PEEK) != 0);
    if (r >= 0 && addr && addrlen)
        socket_inet_addr(addr, addrlen, c->remote_ip, c->remote_port);
    return r;
}

static int tcp_sock_shutdown(socket_t *s, int how) {
    tcp_conn_t *c = s->proto;
    if (!c || (s->state != SS_CONNECTED && s->state != SS_CONNECTING))
        return -NET_ENOTCONN;
    if (how != SHUT_RD)
        tcp_shutdown_write(c);                  // FIN: "I have nothing more to say"
    return 0;
}

static int tcp_sock_name(socket_t *s, int peer, sockaddr_t *addr, uint32_t *len) {
    tcp_conn_t *c = s->proto;
    if (peer && (!c || s->state != SS_CONNECTED))
        return -NET_ENOTCONN;
    if (peer)
        socket_inet_addr(addr, len, c->remote_ip, c->remote_port);
    else
        socket_inet_addr(addr, len, c ? c->local_ip : 0, c ? c->local_port : 0);
    return 0;
}

static int tcp_sock_poll(socket_t *s) {
    tcp_conn_t *c = s->proto;
    if (!c)
        return POLLOUT;
    if (s->state == SS_LISTENING)
        return tcp_ready_children(c) ? POLLIN : 0;

    // A connect() in progress ends here: connected, or refused / timed out.
    if (s->state == SS_CONNECTING) {
        if (c->state == TCP_ESTABLISHED || c->state == TCP_CLOSE_WAIT) {
            s->state = SS_CONNECTED;
        } else if (c->state == TCP_CLOSED) {
            s->state = SS_CLOSED;
            s->error = c->error ? c->error : NET_ECONNREFUSED;
        }
    }
    int bits = 0;
    if (tcp_readable(c))
        bits |= POLLIN;
    if (tcp_writable(c))
        bits |= POLLOUT;
    if (c->state == TCP_CLOSED)
        bits |= POLLHUP | (c->error ? POLLERR : 0);
    return bits;
}

static void tcp_sock_close(socket_t *s) {
    if (s->proto)
        tcp_close(s->proto);                    // TCP finishes it (or frees it)
    s->proto = 0;
}

const socket_ops_t socket_tcp_ops = {
    .bind     = tcp_sock_bind,
    .connect  = tcp_sock_connect,
    .listen   = tcp_sock_listen,
    .accept   = tcp_sock_accept,
    .send     = tcp_sock_send,
    .recv     = tcp_sock_recv,
    .shutdown = tcp_sock_shutdown,
    .name     = tcp_sock_name,
    .poll     = tcp_sock_poll,
    .close    = tcp_sock_close,
};

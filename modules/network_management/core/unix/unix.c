/*
 * unix.c - AF_UNIX streams and datagrams.
 *
 * Stream: each end has a ring buffer; write() puts bytes into the
 * PEER's ring and wakes it. connect() makes a new end for the server
 * side and puts it in the listener's queue, where accept() finds it.
 *
 * Datagram: a message is a packet (core/packet) in the receiving
 * socket's queue, charged against its rcvbuf like a UDP datagram. The
 * sender's path rides along in the packet's headroom (p->net_header),
 * so recvfrom() can say who sent it.
 *
 * Uses: socket_* (core/socket), packet_* (core/packet),
 *       process_caller_uid / scheduler (process_management), NET_E*
 *       (core/inet), strcmp / strncpy / memcpy / memset (lib/string).
 */

#include "modules/network_management/core/unix/unix.h"           // this file
#include "modules/network_management/core/inet/inet.h"           // NET_E*
#include "modules/process_management/core/scheduler/scheduler.h" // scheduler_get_current_process
#include "lib/string/string.h"                                    // strcmp, strncpy, memcpy, memset
#include "modules/network_management/security/unix_impersonation/defence/peercred_check/def_peercred_check.h"  // def_peer_cred

static unix_sock_t ends[UNIX_MAX];

const unix_sock_t *unix_get(int slot) {
    return slot >= 0 && slot < UNIX_MAX && ends[slot].used ? &ends[slot] : 0;
}

/* ---------- Ends ---------- */

static unix_sock_t *new_end(socket_t *s) {
    for (int i = 0; i < UNIX_MAX; i++) {
        if (!ends[i].used) {
            memset(&ends[i], 0, sizeof(ends[i]));
            ends[i].used = 1;
            ends[i].sock = s;
            return &ends[i];
        }
    }
    return 0;
}

static unix_sock_t *end_of(socket_t *s) {
    if (!s->proto)
        s->proto = new_end(s);
    return s->proto;
}

// The bound end named 'path' (of the right type), or NULL.
static unix_sock_t *find(const char *path, int type) {
    for (int i = 0; i < UNIX_MAX; i++)
        if (ends[i].used && ends[i].path[0] && ends[i].sock &&
            ends[i].sock->type == type && strcmp(ends[i].path, path) == 0)
            return &ends[i];
    return 0;
}

// The path out of a sockaddr_un (at most len bytes of it).
static int read_path(const sockaddr_t *addr, uint32_t len, char *path) {
    if (!addr || len <= 2 || addr->family != AF_UNIX)
        return -NET_EINVAL;
    const sockaddr_un_t *un = (const sockaddr_un_t *)addr;
    uint32_t n = len - 2 < UNIX_PATH_MAX - 1 ? len - 2 : UNIX_PATH_MAX - 1;
    memcpy(path, un->path, n);
    path[n] = '\0';
    return path[0] ? 0 : -NET_EINVAL;
}

static void write_path(sockaddr_t *addr, uint32_t *len, const char *path) {
    sockaddr_un_t un;
    memset(&un, 0, sizeof(un));
    un.family = AF_UNIX;
    strncpy(un.path, path, UNIX_PATH_MAX - 1);
    uint32_t n = *len < sizeof(un) ? *len : sizeof(un);
    memcpy(addr, &un, n);
    *len = 2 + strlen(path) + 1;
}

/* ---------- The operations ---------- */

static int unix_bind(socket_t *s, const sockaddr_t *addr, uint32_t len) {
    char path[UNIX_PATH_MAX];
    int r = read_path(addr, len, path);
    if (r < 0)
        return r;
    unix_sock_t *u = end_of(s);
    if (!u)
        return -NET_ENOBUFS;
    if (u->path[0])
        return -NET_EINVAL;                     // bound already
    for (int i = 0; i < UNIX_MAX; i++)
        if (ends[i].used && strcmp(ends[i].path, path) == 0)
            return -NET_EADDRINUSE;
    strcpy(u->path, path);
    return 0;
}

static int unix_listen(socket_t *s, int backlog) {
    unix_sock_t *u = end_of(s);
    if (!u || !u->path[0] || s->type != SOCK_STREAM)
        return !u ? -NET_ENOBUFS : -NET_EINVAL;
    u->backlog = backlog < 1 ? 1 : backlog > UNIX_BACKLOG ? UNIX_BACKLOG : backlog;
    s->state = SS_LISTENING;
    return 0;
}

// Join two ends; each remembers who the other was.
static void join(unix_sock_t *a, unix_sock_t *b, socket_cred_t cred_a, socket_cred_t cred_b) {
    a->peer = b;
    b->peer = a;
    a->peer_cred = cred_b;
    b->peer_cred = cred_a;
    a->sock->state = SS_CONNECTED;
    if (b->sock)
        b->sock->state = SS_CONNECTED;
}

static int unix_connect(socket_t *s, const sockaddr_t *addr, uint32_t len) {
    char path[UNIX_PATH_MAX];
    int r = read_path(addr, len, path);
    if (r < 0)
        return r;
    unix_sock_t *u = end_of(s);
    if (!u)
        return -NET_ENOBUFS;
    unix_sock_t *target = find(path, s->type);
    if (!target)
        return -NET_ECONNREFUSED;               // nobody bound there
    if (s->type == SOCK_DGRAM) {                // datagram: only remember the peer
        u->peer = target;
        u->peer_cred = target->peer_cred;
        s->state = SS_CONNECTED;
        return 0;
    }
    if (target->sock->state != SS_LISTENING)
        return -NET_ECONNREFUSED;
    if (target->queued >= target->backlog)
        return -NET_EAGAIN;                     // its queue is full: wait
    unix_sock_t *server_end = new_end(0);       // its socket is made by accept()
    if (!server_end)
        return -NET_ENOBUFS;
    // DEFENCE peercred_check: each side's credentials are recorded by the
    // kernel now, from the sockets' owners; nothing a peer says changes them.
    server_end->peer = u;
    u->peer = server_end;
    u->peer_cred = def_peer_cred(target->sock);
    server_end->peer_cred = def_peer_cred(s);
    s->state = SS_CONNECTED;
    target->queue[target->queued++] = server_end;
    socket_wake(target->sock);
    return 0;
}

static int unix_accept(socket_t *s, socket_t **child) {
    unix_sock_t *l = s->proto;
    if (!l || s->state != SS_LISTENING)
        return -NET_EINVAL;
    if (!l->queued)
        return -NET_EAGAIN;
    socket_t *ns;
    int r = socket_create(AF_UNIX, SOCK_STREAM, 0, &ns);
    if (r < 0)
        return r;
    unix_sock_t *end = l->queue[0];
    for (int i = 1; i < l->queued; i++)
        l->queue[i - 1] = l->queue[i];
    l->queued--;
    end->sock = ns;
    ns->proto = end;
    ns->state = SS_CONNECTED;
    *child = ns;
    return 0;
}

static int unix_send(socket_t *s, const void *buf, uint32_t len, int flags,
                     const sockaddr_t *addr, uint32_t addrlen) {
    (void)flags;
    unix_sock_t *u = end_of(s);
    if (!u)
        return -NET_ENOBUFS;
    if (s->type == SOCK_DGRAM) {
        unix_sock_t *to = u->peer;
        if (addr) {
            char path[UNIX_PATH_MAX];
            int r = read_path(addr, addrlen, path);
            if (r < 0)
                return r;
            to = find(path, SOCK_DGRAM);
        }
        if (!to || !to->sock)
            return addr ? -NET_ECONNREFUSED : -NET_ENOTCONN;
        if (len > PACKET_DATA_MAX)
            return -NET_EMSGSIZE;
        packet_t *p = packet_from(buf, (uint16_t)len);
        if (!p)
            return -NET_ENOBUFS;
        // The sender's name rides in the headroom, for recvfrom().
        strncpy((char *)p->buf, u->path, UNIX_PATH_MAX - 1);
        p->buf[UNIX_PATH_MAX - 1] = '\0';
        p->net_header = p->buf;
        return socket_queue(to->sock, p) ? (int)len : -NET_EAGAIN;
    }
    // Stream: into the peer's ring.
    unix_sock_t *peer = u->peer;
    if (!peer || u->peer_gone)
        return -NET_EPIPE;
    uint32_t room = UNIX_BUF_SIZE - peer->len;
    if (!room)
        return -NET_EAGAIN;
    uint32_t n = len < room ? len : room;
    const uint8_t *bytes = buf;
    for (uint32_t i = 0; i < n; i++)
        peer->buf[(peer->head + peer->len + i) % UNIX_BUF_SIZE] = bytes[i];
    peer->len += n;
    if (peer->sock)
        socket_wake(peer->sock);
    return (int)n;
}

static int unix_recv(socket_t *s, void *buf, uint32_t len, int flags,
                     sockaddr_t *addr, uint32_t *addrlen) {
    unix_sock_t *u = end_of(s);
    if (!u)
        return -NET_ENOBUFS;
    if (s->type == SOCK_DGRAM) {
        packet_t *p = (flags & MSG_PEEK) ? packet_queue_peek(&s->rx) : packet_queue_pop(&s->rx);
        if (!p)
            return -NET_EAGAIN;
        uint32_t n = p->len < len ? p->len : len;
        memcpy(buf, p->data, n);
        if (addr && addrlen)
            write_path(addr, addrlen, (const char *)p->net_header);
        if (!(flags & MSG_PEEK)) {
            s->rx_charged -= SOCKET_TRUESIZE;
            packet_free(p);
        }
        return (int)n;
    }
    if (!u->len) {
        if (u->peer_gone)
            return 0;                           // the peer closed: end of the stream
        return u->peer ? -NET_EAGAIN : -NET_ENOTCONN;
    }
    uint32_t n = len < u->len ? len : u->len;
    uint8_t *out = buf;
    for (uint32_t i = 0; i < n; i++)
        out[i] = u->buf[(u->head + i) % UNIX_BUF_SIZE];
    if (!(flags & MSG_PEEK)) {
        u->head = (u->head + n) % UNIX_BUF_SIZE;
        u->len -= n;
        if (u->peer && u->peer->sock)
            socket_wake(u->peer->sock);         // room again for a writer that waits
    }
    if (addr && addrlen && u->peer)
        write_path(addr, addrlen, u->peer->path);
    return (int)n;
}

static int unix_shutdown(socket_t *s, int how) {
    unix_sock_t *u = s->proto;
    if (u && u->peer && how != SHUT_RD) {
        u->peer->peer_gone = 1;                 // the peer reads EOF after the bytes
        if (u->peer->sock)
            socket_wake(u->peer->sock);
    }
    return 0;
}

static int unix_name(socket_t *s, int peer, sockaddr_t *addr, uint32_t *len) {
    unix_sock_t *u = end_of(s);
    if (peer && (!u || !u->peer))
        return -NET_ENOTCONN;
    write_path(addr, len, u ? (peer ? u->peer->path : u->path) : "");
    return 0;
}

static int unix_poll(socket_t *s) {
    unix_sock_t *u = s->proto;
    if (!u)
        return POLLOUT;
    if (s->state == SS_LISTENING)
        return u->queued ? POLLIN : 0;
    if (s->type == SOCK_DGRAM)
        return POLLOUT | (s->rx.count ? POLLIN : 0);
    int bits = 0;
    if (u->len || u->peer_gone)
        bits |= POLLIN;
    if (u->peer_gone)
        bits |= POLLHUP;
    else if (u->peer && u->peer->len < UNIX_BUF_SIZE)
        bits |= POLLOUT;
    return bits;
}

static void unix_close(socket_t *s) {
    unix_sock_t *u = s->proto;
    if (!u)
        return;
    // Connections waiting in a closing listener's queue are dropped.
    for (int i = 0; i < u->queued; i++) {
        unix_sock_t *w = u->queue[i];
        if (w->peer) {
            w->peer->peer_gone = 1;
            w->peer->peer = 0;
            if (w->peer->sock)
                socket_wake(w->peer->sock);
        }
        w->used = 0;
    }
    if (u->peer && u->peer->peer == u) {        // tell the other end
        u->peer->peer_gone = 1;
        u->peer->peer = 0;
        if (u->peer->sock)
            socket_wake(u->peer->sock);
    }
    // A datagram socket connected to us forgets us too.
    for (int i = 0; i < UNIX_MAX; i++)
        if (ends[i].used && ends[i].peer == u)
            ends[i].peer = 0;
    u->used = 0;
    s->proto = 0;
}

int unix_pair(socket_t *a, socket_t *b) {
    unix_sock_t *ua = end_of(a), *ub = end_of(b);
    if (!ua || !ub)
        return -NET_ENOBUFS;
    join(ua, ub, def_peer_cred(a), def_peer_cred(b));   // DEFENCE peercred_check
    return 0;
}

int unix_peer_cred(socket_t *s, socket_cred_t *out) {
    unix_sock_t *u = s->proto;
    if (!u || !u->peer)
        return -NET_ENOTCONN;
    *out = u->peer_cred;
    return 0;
}

const socket_ops_t socket_unix_ops = {
    .bind     = unix_bind,
    .connect  = unix_connect,
    .listen   = unix_listen,
    .accept   = unix_accept,
    .send     = unix_send,
    .recv     = unix_recv,
    .shutdown = unix_shutdown,
    .name     = unix_name,
    .poll     = unix_poll,
    .close    = unix_close,
};

/*
 * socket.c - The socket table, sockfs, options, and waiting.
 *
 * Every operation runs the protocol's function with the network locked
 * (interrupts off), because the card's interrupt changes the same
 * queues. If the answer is -NET_EAGAIN ("not yet") and the socket may
 * block, the process sleeps a while (unlocked!) and tries again.
 * socket_wake(), called by the protocol when something arrives, ends
 * the sleep at once; the slice is only the longest possible delay.
 *
 * Uses: netdev_lock (core/netdev), socket_udp_ops / socket_tcp_ops / socket_raw_ops (sock_*.c),
 *       socket_unix_ops / unix_peer_cred (core/unix), fd_alloc
 *       (io_management), caps_caller_has (security_management),
 *       process_caller_uid / scheduler (process_management),
 *       sys_sleep_interruptible (process_management), format, memset, memcpy.
 */

#include "modules/network_management/core/socket/socket.h"       // this file
#include "modules/network_management/core/netdev/netdev.h"      // netdev_lock, netdev_unlock
#include "modules/network_management/core/inet/inet.h"          // NET_E*, htons, htonl
#include "modules/network_management/core/unix/unix.h"          // socket_unix_ops, unix_peer_cred
#include "modules/network_management/core/tcp/tcp.h"            // tcp_conn_t (FIONREAD)
#include "modules/io_management/core/fd/fd.h"                   // fd_alloc
#include "modules/io_management/syscalls/open/sys_open.h"       // O_READ, O_WRITE
#include "modules/security_management/core/caps/caps.h"         // caps_caller_has, CAP_NET_RAW
#include "modules/process_management/core/scheduler/scheduler.h"     // scheduler_get_current_process
#include "modules/process_management/syscalls/sleep/sys_sleep.h"     // sys_sleep_interruptible
#include "lib/format/format.h"                                  // format
#include "lib/string/string.h"                                  // memset, memcpy
#include "modules/network_management/security/raw_socket_abuse/defence/raw_cap/def_raw_cap.h" // def_raw_socket_ok
#include "modules/network_management/security/sockbuf_exhaustion/defence/rmem_limit/def_rmem_limit.h" // def_rmem_ok
#include "modules/network_management/security/sockopt_overflow/defence/optlen_check/def_optlen_check.h" // def_optlen_ok
#include "modules/network_management/security/sockaddr_leak/defence/sockaddr_zero/def_sockaddr_zero.h" // def_sockaddr_wipe

#define WAIT_SLICE_MS 50            // longest nap between two tries
#define RCVBUF_MIN    (2 * SOCKET_TRUESIZE)
#define RCVBUF_MAX    (64 * SOCKET_TRUESIZE)

static socket_t sockets[SOCKET_MAX];
static uint32_t next_ino = 1;

/* ---------- sockfs: the vfs side ---------- */

// read() / write() on a socket descriptor = recv() / send() with no address.
static uint32_t sockfs_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buf) {
    (void)offset;
    return (uint32_t)socket_recvfrom((socket_t *)node, buf, size, 0, 0, 0);
}

static uint32_t sockfs_write(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buf) {
    (void)offset;
    return (uint32_t)socket_sendto((socket_t *)node, buf, size, 0, 0, 0);
}

// The last descriptor was closed (vfs_close): end the conversation.
static void sockfs_release(vfs_node_t *node) {
    socket_close((socket_t *)node);
}

int socket_is(const vfs_node_t *node) {
    return node >= &sockets[0].node && node <= &sockets[SOCKET_MAX - 1].node;
}

/* ---------- Making them ---------- */

static const socket_ops_t *ops_for(int family, int type, int protocol) {
    if (family == AF_UNIX)
        return (type == SOCK_STREAM || type == SOCK_DGRAM) && protocol == 0 ? &socket_unix_ops : 0;
    if (family != AF_INET)
        return 0;
    if (type == SOCK_DGRAM && (protocol == IPPROTO_IP || protocol == 17))
        return &socket_udp_ops;
    if (type == SOCK_STREAM && (protocol == IPPROTO_IP || protocol == 6))
        return &socket_tcp_ops;
    if (type == SOCK_RAW && protocol > 0 && protocol < 255)
        return &socket_raw_ops;
    return 0;
}

int socket_create(int family, int type, int protocol, socket_t **out) {
    int nonblock = (type & SOCK_NONBLOCK) != 0;
    type &= SOCK_TYPE_MASK;
    if (family != AF_INET && family != AF_UNIX)
        return -NET_EAFNOSUPPORT;
    if (type == SOCK_RAW && !def_raw_socket_ok(process_caller_uid()))
        return -NET_EPERM;                      // DEFENCE raw_cap: raw packets can be forged
    const socket_ops_t *ops = ops_for(family, type, protocol);
    if (!ops)
        return type == SOCK_STREAM || type == SOCK_DGRAM || type == SOCK_RAW
               ? -NET_EPROTONOSUPPORT : -NET_EINVAL;

    uint32_t lock = netdev_lock();
    socket_t *s = 0;
    for (int i = 0; i < SOCKET_MAX && !s; i++)
        if (!sockets[i].used && sockets[i].node.open_count == 0)
            s = &sockets[i];
    if (!s) {
        netdev_unlock(lock);
        return -NET_ENOBUFS;
    }
    memset(s, 0, sizeof(*s));
    s->used = 1;
    netdev_unlock(lock);

    s->family = family;
    s->type = type;
    s->protocol = protocol;
    s->ops = ops;
    s->nonblock = nonblock;
    s->uid = process_caller_uid();
    process_t *self = scheduler_get_current_process();
    s->pid = self ? self->pid : 0;
    s->rcvbuf = SOCKET_RCVBUF_DEFAULT;
    s->sndbuf = SOCKET_SNDBUF_DEFAULT;

    // The vfs node: no name in any directory, freed with its last descriptor.
    format(s->node.name, sizeof(s->node.name), "socket:[%u]", next_ino);
    s->node.ino = next_ino++;
    s->node.flags = FS_FILE;
    s->node.owner = s->uid;
    s->node.mode = 066;                         // the owner and others may read+write it
    s->node.read = sockfs_read;
    s->node.write = sockfs_write;
    s->node.release = sockfs_release;
    s->node.parent = 0;                         // in no directory: release() when unused
    s->node.nlink = 0;
    *out = s;
    return 0;
}

int socket_install_fd(process_t *p, socket_t *s) {
    int fd = fd_alloc(p, &s->node, O_READ | O_WRITE);
    return fd < 0 ? -NET_EMFILE : fd;
}

socket_t *socket_from_fd(process_t *p, int fd) {
    if (!p || fd < 0 || fd >= MAX_FDS || !socket_is(p->fd_table[fd]))
        return 0;
    return (socket_t *)p->fd_table[fd];
}

int socket_fd_error(process_t *p, int fd) {
    if (!p || fd < 0 || fd >= MAX_FDS || !p->fd_table[fd])
        return -NET_EBADF;
    return -NET_ENOTSOCK;
}

void socket_close(socket_t *s) {
    uint32_t lock = netdev_lock();
    if (s->used) {
        if (s->ops->close)
            s->ops->close(s);
        packet_queue_flush(&s->rx);
        s->used = 0;
    }
    netdev_unlock(lock);
}

const socket_t *socket_get(int slot) {
    return slot >= 0 && slot < SOCKET_MAX && sockets[slot].used ? &sockets[slot] : 0;
}

/* ---------- Waiting ---------- */

void socket_wake(socket_t *s) {
    process_t *p = s->waiter;
    if (p && p->state == PROCESS_STATE_SLEEPING && p->sleep_interruptible) {
        p->sleep_ticks = 0;
        p->state = PROCESS_STATE_READY;         // as a signal would wake it
    }
}

// "Not yet": may we sleep and try again? 0 = tried again after a nap,
// or why not: -NET_EAGAIN (non-blocking, or the timeout ran out),
// -NET_EINTR (a signal came).
static int nap(socket_t *s, int flags, uint32_t timeout_ms, uint32_t *waited) {
    if (s->nonblock || (flags & MSG_DONTWAIT))
        return -NET_EAGAIN;
    if (timeout_ms && *waited >= timeout_ms)
        return -NET_EAGAIN;                     // SO_RCVTIMEO / SO_SNDTIMEO ran out
    s->waiter = scheduler_get_current_process();
    int calm = sys_sleep_interruptible(WAIT_SLICE_MS);
    s->waiter = 0;
    *waited += WAIT_SLICE_MS;
    return calm ? 0 : -NET_EINTR;
}

/* ---------- The operations ---------- */

int socket_bind(socket_t *s, const sockaddr_t *addr, uint32_t len) {
    if (!s->ops->bind)
        return -NET_EOPNOTSUPP;
    uint32_t lock = netdev_lock();
    int r = s->ops->bind(s, addr, len);
    netdev_unlock(lock);
    return r;
}

int socket_connect(socket_t *s, const sockaddr_t *addr, uint32_t len) {
    if (!s->ops->connect)
        return -NET_EOPNOTSUPP;
    uint32_t lock = netdev_lock();
    int r = s->ops->connect(s, addr, len);
    netdev_unlock(lock);
    if (r != -NET_EINPROGRESS || s->nonblock)
        return r;

    // A connection is being made (TCP): wait for the answer.
    uint32_t waited = 0;
    for (;;) {
        lock = netdev_lock();
        int ready = socket_poll(s) & (POLLOUT | POLLERR | POLLHUP);
        int err = s->error;
        s->error = 0;
        netdev_unlock(lock);
        if (ready)
            return err ? -err : 0;
        int why = nap(s, 0, s->sndtimeo_ms, &waited);
        if (why)
            return why == -NET_EAGAIN ? -NET_ETIMEDOUT : why;
    }
}

int socket_listen(socket_t *s, int backlog) {
    if (!s->ops->listen)
        return -NET_EOPNOTSUPP;
    uint32_t lock = netdev_lock();
    int r = s->ops->listen(s, backlog);
    netdev_unlock(lock);
    return r;
}

int socket_accept(socket_t *s, socket_t **child) {
    if (!s->ops->accept)
        return -NET_EOPNOTSUPP;
    uint32_t waited = 0;
    for (;;) {
        uint32_t lock = netdev_lock();
        int r = s->ops->accept(s, child);
        netdev_unlock(lock);
        if (r != -NET_EAGAIN)
            return r;
        int why = nap(s, 0, s->rcvtimeo_ms, &waited);
        if (why)
            return why;
    }
}

// A stream takes what fits in its buffer; a blocking send() goes on
// until ALL of 'len' is taken (as in Linux). A datagram goes whole or not.
int socket_sendto(socket_t *s, const void *buf, uint32_t len, int flags,
                  const sockaddr_t *addr, uint32_t addrlen) {
    if (s->shut_wr)
        return -NET_EPIPE;
    uint32_t waited = 0, done = 0;
    const uint8_t *bytes = buf;
    for (;;) {
        uint32_t lock = netdev_lock();
        int r = s->ops->send(s, bytes + done, len - done, flags, addr, addrlen);
        netdev_unlock(lock);
        if (r > 0 && s->type == SOCK_STREAM) {
            done += (uint32_t)r;
            if (done == len || s->nonblock || (flags & MSG_DONTWAIT))
                return (int)done;
            continue;                           // more to go: try again (maybe after a nap)
        }
        if (r != -NET_EAGAIN)
            return done && r < 0 ? (int)done : r;
        int why = nap(s, flags, s->sndtimeo_ms, &waited);
        if (why)
            return done ? (int)done : why;
    }
}

int socket_recvfrom(socket_t *s, void *buf, uint32_t len, int flags,
                    sockaddr_t *addr, uint32_t *addrlen) {
    uint32_t waited = 0;
    for (;;) {
        uint32_t lock = netdev_lock();
        int r = s->shut_rd && !s->rx.count ? 0 : s->ops->recv(s, buf, len, flags, addr, addrlen);
        netdev_unlock(lock);
        if (r != -NET_EAGAIN)
            return r;
        int why = nap(s, flags, s->rcvtimeo_ms, &waited);
        if (why)
            return why;
    }
}

int socket_shutdown(socket_t *s, int how) {
    if (how < SHUT_RD || how > SHUT_RDWR)
        return -NET_EINVAL;
    uint32_t lock = netdev_lock();
    int r = s->ops->shutdown ? s->ops->shutdown(s, how) : 0;
    if (how != SHUT_WR)
        s->shut_rd = 1;
    if (how != SHUT_RD)
        s->shut_wr = 1;
    socket_wake(s);
    netdev_unlock(lock);
    return r;
}

int socket_getname(socket_t *s, int peer, sockaddr_t *addr, uint32_t *len) {
    if (!s->ops->name)
        return -NET_EOPNOTSUPP;
    uint32_t lock = netdev_lock();
    int r = s->ops->name(s, peer, addr, len);
    netdev_unlock(lock);
    return r;
}

int socket_poll(socket_t *s) {
    int bits = s->ops->poll ? s->ops->poll(s) : 0;
    if (s->error)
        bits |= POLLERR;
    return bits;
}

int socket_pending_bytes(socket_t *s) {
    uint32_t lock = netdev_lock();
    int n = 0;
    packet_t *first = packet_queue_peek(&s->rx);
    if (first)
        n = first->len;                         // datagrams: the next one
    else if (s->proto && s->family == AF_INET && s->type == SOCK_STREAM)
        n = (int)((tcp_conn_t *)s->proto)->rcv_len;
    else if (s->proto && s->family == AF_UNIX && s->type == SOCK_STREAM)
        n = (int)((unix_sock_t *)s->proto)->len;
    netdev_unlock(lock);
    return n;
}

/* ---------- Options ---------- */

// An int option: the value must be at least 4 bytes long.
static int read_int(const void *value, uint32_t len, int *out) {
    if (!value || len < sizeof(int))
        return -NET_EINVAL;
    *out = *(const int *)value;
    return 0;
}

static int read_timeout(const void *value, uint32_t len, uint32_t *ms) {
    if (!value || len < sizeof(socket_timeval_t))
        return -NET_EINVAL;
    const socket_timeval_t *tv = value;
    *ms = tv->sec * 1000 + tv->usec / 1000;
    return 0;
}

int socket_setopt(socket_t *s, int level, int name, const void *value, uint32_t len) {
    int v = 0;
    if (!value || !def_optlen_ok(level, name, len))
        return -NET_EINVAL;                     // DEFENCE optlen_check: never read past 'len'
    if (level == SOL_SOCKET && (name == SO_RCVTIMEO || name == SO_SNDTIMEO))
        return read_timeout(value, len, name == SO_RCVTIMEO ? &s->rcvtimeo_ms : &s->sndtimeo_ms);
    if (read_int(value, len, &v) < 0)
        return -NET_EINVAL;

    if (level == SOL_SOCKET) {
        switch (name) {
        case SO_REUSEADDR: s->reuseaddr = v != 0; return 0;
        case SO_REUSEPORT: s->reuseport = v != 0; return 0;
        case SO_BROADCAST: s->broadcast = v != 0; return 0;
        case SO_KEEPALIVE: s->keepalive = v != 0; return 0;
        case SO_RCVBUF:
            s->rcvbuf = v < (int)RCVBUF_MIN ? RCVBUF_MIN : v > (int)RCVBUF_MAX ? RCVBUF_MAX : (uint32_t)v;
            return 0;
        case SO_SNDBUF:
            s->sndbuf = v < 1024 ? 1024 : v > 65536 ? 65536 : (uint32_t)v;
            return 0;
        }
    } else if (level == SOL_IP && name == IP_TTL) {
        if (v < 1 || v > 255)
            return -NET_EINVAL;
        s->ttl = (uint8_t)v;
        return 0;
    } else if (level == SOL_TCP && name == TCP_NODELAY) {
        s->nodelay = v != 0;
        return 0;
    }
    return -NET_ENOPROTOOPT;
}

int socket_getopt(socket_t *s, int level, int name, void *value, uint32_t *len) {
    int v;
    if (level == SOL_SOCKET && (name == SO_RCVTIMEO || name == SO_SNDTIMEO)) {
        if (*len < sizeof(socket_timeval_t))
            return -NET_EINVAL;
        uint32_t ms = name == SO_RCVTIMEO ? s->rcvtimeo_ms : s->sndtimeo_ms;
        socket_timeval_t tv = { ms / 1000, (ms % 1000) * 1000 };
        memcpy(value, &tv, sizeof(tv));
        *len = sizeof(tv);
        return 0;
    }
    if (level == SOL_SOCKET && name == SO_PEERCRED) {
        socket_cred_t cred;
        if (s->family != AF_UNIX)
            return -NET_ENOPROTOOPT;            // only a local peer can be known
        if (*len < sizeof(cred))
            return -NET_EINVAL;
        int r = unix_peer_cred(s, &cred);
        if (r < 0)
            return r;
        memcpy(value, &cred, sizeof(cred));
        *len = sizeof(cred);
        return 0;
    }
    if (level == SOL_SOCKET) {
        switch (name) {
        case SO_TYPE:       v = s->type; break;
        case SO_ERROR:      v = s->error; s->error = 0; break;
        case SO_REUSEADDR:  v = s->reuseaddr; break;
        case SO_REUSEPORT:  v = s->reuseport; break;
        case SO_BROADCAST:  v = s->broadcast; break;
        case SO_KEEPALIVE:  v = s->keepalive; break;
        case SO_RCVBUF:     v = (int)s->rcvbuf; break;
        case SO_SNDBUF:     v = (int)s->sndbuf; break;
        case SO_ACCEPTCONN: v = s->state == SS_LISTENING; break;
        default:            return -NET_ENOPROTOOPT;
        }
    } else if (level == SOL_IP && name == IP_TTL) {
        v = s->ttl ? s->ttl : 64;
    } else if (level == SOL_TCP && name == TCP_NODELAY) {
        v = s->nodelay;
    } else {
        return -NET_ENOPROTOOPT;
    }
    if (*len < sizeof(int))
        return -NET_EINVAL;
    memcpy(value, &v, sizeof(int));
    *len = sizeof(int);
    return 0;
}

/* ---------- Helpers for the protocols ---------- */

void socket_raw_deliver(packet_t *p) {
    const uint8_t protocol = p->data[9];        // the IP header's protocol byte
    for (int i = 0; i < SOCKET_MAX; i++) {
        socket_t *s = &sockets[i];
        if (!s->used || s->type != SOCK_RAW || s->protocol != protocol)
            continue;
        packet_t *copy = packet_from(p->data, p->len);
        if (copy)
            socket_queue(s, copy);              // wakes it, or drops if its queue is full
    }
}

int socket_queue(socket_t *s, packet_t *p) {
    // DEFENCE rmem_limit: a datagram costs its whole buffer, up to rcvbuf.
    if (s->shut_rd || !def_rmem_ok(s->rx_charged, SOCKET_TRUESIZE, s->rcvbuf)) {
        s->drops++;                             // the program does not read fast enough
        packet_free(p);
        return 0;
    }
    s->rx_charged += SOCKET_TRUESIZE;
    packet_queue_push(&s->rx, p);
    socket_wake(s);
    return 1;
}

void socket_inet_addr(sockaddr_t *out, uint32_t *len, uint32_t ip, uint16_t port) {
    sockaddr_in_t in;
    def_sockaddr_wipe(&in, sizeof(in));         // DEFENCE sockaddr_zero: no leftover bytes
    in.family = AF_INET;
    in.port = htons(port);
    in.addr = htonl(ip);
    uint32_t n = *len < sizeof(in) ? *len : sizeof(in);
    memcpy(out, &in, n);                        // as much as the caller has room for
    *len = sizeof(in);
}

int socket_inet_parse(const sockaddr_t *addr, uint32_t len, uint32_t *ip, uint16_t *port) {
    if (!addr || len < sizeof(sockaddr_in_t))
        return -NET_EINVAL;
    const sockaddr_in_t *in = (const sockaddr_in_t *)addr;
    if (in->family != AF_INET)
        return -NET_EAFNOSUPPORT;
    *ip = ntohl(in->addr);
    *port = ntohs(in->port);
    return 0;
}

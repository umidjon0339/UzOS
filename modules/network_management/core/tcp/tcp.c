/*
 * tcp.c - The TCP state machine: what to do with each arriving segment.
 *
 * The rules come from RFC 793 ("segment arrives"), kept simple:
 *   - data must arrive in order: a segment after a gap is dropped (its
 *     sender will retransmit it), so no reassembly queue is needed;
 *   - every segment with data or FIN is acknowledged at once (no
 *     delayed ACKs);
 *   - RST and SYN in an open connection follow RFC 5961: a RST counts
 *     only at exactly the expected sequence number, anything else gets
 *     a "challenge ACK" (security/rst_injection);
 *   - initial sequence numbers follow RFC 6528: unguessable
 *     (security/tcp_seq_prediction);
 *   - a full listen queue answers with SYN cookies (security/syn_flood).
 *
 * Every rule that stops an attack is a DEFENCE in security/; this file
 * marks where each one is asked.
 *
 * Uses: packet_* (core/packet), ipv4_send / ipv4_header (core/ipv4),
 *       route_lookup / route_is_local (core/route), socket_wake
 *       (core/socket), inet_checksum_pseudo / htons (core/inet),
 *       net_now_ms (core/net_tick), tsc_now (cpu_management), memcpy.
 */

#include "modules/network_management/core/tcp/tcp.h"             // this file
#include "modules/network_management/core/ipv4/ipv4.h"           // ipv4_send, ipv4_header, IPPROTO_TCP
#include "modules/network_management/core/route/route.h"         // route_lookup
#include "modules/network_management/core/socket/socket.h"       // socket_wake
#include "modules/network_management/core/inet/inet.h"           // inet_checksum_pseudo, htons, NET_E*
#include "modules/network_management/core/net_tick/net_tick.h"   // net_now_ms
#include "modules/cpu_management/core/tsc/tsc.h"                 // tsc_now
#include "lib/string/string.h"                                    // memcpy, memset
#include "modules/network_management/security/syn_flood/defence/syn_cookies/def_syn_cookies.h" // def_syn_cookie_make, def_syn_cookie_check
#include "modules/network_management/security/tcp_seq_prediction/defence/random_isn/def_random_isn.h" // def_isn
#include "modules/network_management/security/rst_injection/defence/rst_window_check/def_rst_window_check.h" // def_rst_exact_ok
#include "modules/network_management/security/tcp_session_hijack/defence/seq_window_check/def_seq_window_check.h" // def_seq_in_window
#include "modules/network_management/security/xmas_null_scan/defence/tcp_flag_sanity/def_tcp_flag_sanity.h" // def_tcp_flags_ok
#include "modules/network_management/security/port_scan/defence/scan_detect/def_scan_detect.h" // def_scan_rst_ok
#include "modules/network_management/security/sack_panic/defence/mss_min/def_mss_min.h" // def_mss_clamp
#include "modules/network_management/security/connection_exhaustion/defence/conn_quota/def_conn_quota.h" // def_conn_ok
#include "modules/network_management/security/time_wait_assassination/defence/rfc1337/def_rfc1337.h" // def_timewait_rst_ok
#include "modules/network_management/core/route/route.h"                        // route_is_local

#define OUR_MSS        1460             // 1500 (Ethernet MTU) - 20 (IP) - 20 (TCP)
#define OPT_MSS        2                // option kind: maximum segment size
#define OPT_END        0
#define OPT_NOP        1
#define EPHEMERAL_LOW  32768
#define EPHEMERAL_HIGH 60999
#define CHALLENGE_ACKS 4                // answers to unusable segments, per NET_TICK_MS

static tcp_conn_t conns[TCP_MAX_CONNS];
static tcp_stats_t stats;
static uint16_t next_port = EPHEMERAL_LOW;

tcp_stats_t *tcp_stats(void) { return &stats; }

const tcp_conn_t *tcp_conn(int slot) {
    return slot >= 0 && slot < TCP_MAX_CONNS && conns[slot].used ? &conns[slot] : 0;
}

tcp_conn_t *tcp_slot(int slot) {
    return &conns[slot];
}

const char *tcp_state_name(tcp_state_t state) {
    static const char *names[] = {
        "CLOSED", "LISTEN", "SYN_SENT", "SYN_RECV", "ESTABLISHED", "FIN_WAIT1",
        "FIN_WAIT2", "CLOSE_WAIT", "CLOSING", "LAST_ACK", "TIME_WAIT",
    };
    return names[state];
}

/* ---------- Connections ---------- */

void tcp_set_state(tcp_conn_t *c, tcp_state_t state) {
    c->state = state;
    c->state_since = net_now_ms();
}

// Tell the program (or the listener, for a connection not accepted yet).
static void wake(tcp_conn_t *c) {
    if (c->sock)
        socket_wake(c->sock);
    else if (c->listener && c->listener->sock)
        socket_wake(c->listener->sock);
}

void tcp_free(tcp_conn_t *c) {
    if (c->listener)
        c->listener->pending--;         // it was waiting in the listener's queue
    memset(c, 0, sizeof(*c));
}

// The connection is over. A socket still holding it sees CLOSED and the
// error; nobody holding it: free it now.
void tcp_finish(tcp_conn_t *c, int error) {
    tcp_set_state(c, TCP_CLOSED);
    c->error = error;
    c->rtx_at = 0;
    if (c->sock)
        wake(c);
    else
        tcp_free(c);
}

tcp_conn_t *tcp_new(uint32_t uid) {
    int owned = 0;
    for (int i = 0; i < TCP_MAX_CONNS; i++)
        owned += conns[i].used && conns[i].uid == uid;
    if (!def_conn_ok(uid, owned))               // DEFENCE conn_quota: a share per user
        return 0;
    for (int i = 0; i < TCP_MAX_CONNS; i++) {
        if (!conns[i].used) {
            tcp_conn_t *c = &conns[i];
            memset(c, 0, sizeof(*c));
            c->used = 1;
            c->uid = uid;
            c->mss = TCP_DEFAULT_MSS;
            c->rto_ms = TCP_RTO_INITIAL;
            tcp_set_state(c, TCP_CLOSED);
            return c;
        }
    }
    return 0;
}

// DEFENCE random_isn (RFC 6528): a clock plus a keyed hash of the addresses.
static uint32_t new_isn(const tcp_conn_t *c) {
    return def_isn(c->local_ip, c->local_port, c->remote_ip, c->remote_port);
}

/* ---------- Sending segments ---------- */

static uint16_t receive_window(const tcp_conn_t *c) {
    return (uint16_t)(TCP_BUF_SIZE - c->rcv_len);
}

// Build and send one segment. 'mss' adds the MSS option (SYNs).
static int send_raw(uint32_t src, uint32_t dst, uint16_t sport, uint16_t dport,
                    uint32_t seq, uint32_t ack, uint8_t flags, uint16_t window,
                    int mss, const uint8_t *data, uint16_t len) {
    packet_t *p = packet_alloc();
    if (!p)
        return -NET_ENOBUFS;
    uint16_t hlen = TCP_HLEN + (mss ? 4 : 0);
    tcp_header_t *h = (tcp_header_t *)packet_put(p, hlen);
    if (len)
        memcpy(packet_put(p, len), data, len);
    h->src_port = htons(sport);
    h->dst_port = htons(dport);
    h->seq = htonl(seq);
    h->ack = htonl(ack);
    h->offset = (uint8_t)((hlen / 4) << 4);
    h->flags = flags;
    h->window = htons(window);
    h->checksum = 0;
    h->urgent = 0;
    if (mss) {
        uint8_t *o = (uint8_t *)(h + 1);
        o[0] = OPT_MSS;
        o[1] = 4;
        o[2] = OUR_MSS >> 8;
        o[3] = OUR_MSS & 0xFF;
    }
    h->checksum = inet_checksum_pseudo(src, dst, IPPROTO_TCP, p->data, p->len);
    stats.out_segs++;
    if (flags & TCP_RST)
        stats.out_rsts++;
    return ipv4_send(p, src, dst, IPPROTO_TCP, 0);
}

int tcp_send_segment(tcp_conn_t *c, uint8_t flags, uint32_t seq, const uint8_t *data, uint16_t len) {
    c->segs_out++;
    return send_raw(c->local_ip, c->remote_ip, c->local_port, c->remote_port, seq,
                    (flags & TCP_ACK) ? c->rcv_nxt : 0, flags, receive_window(c),
                    (flags & TCP_SYN) != 0, data, len);
}

static void send_ack(tcp_conn_t *c) {
    tcp_send_segment(c, TCP_ACK, c->snd_nxt, 0, 0);
}

// An ACK that answers a segment we could not use (out of window, a RST
// or SYN that did not fit). At most CHALLENGE_ACKS per NET_TICK_MS:
// two confused ends would otherwise answer each other's ACKs forever
// (RFC 5961 limits them the same way).
static void challenge_ack(tcp_conn_t *c) {
    uint32_t now = net_now_ms();
    if (now - c->challenge_since >= NET_TICK_MS) {
        c->challenge_since = now;
        c->challenges = 0;
    }
    if (c->challenges++ < CHALLENGE_ACKS)
        send_ack(c);
}

static void arm_timer(tcp_conn_t *c) {
    if (!c->rtx_at)
        c->rtx_at = net_now_ms() + c->rto_ms;
}

// Send whatever the window allows: new data, then the FIN when all of
// the data is out.
void tcp_output(tcp_conn_t *c) {
    if (c->state != TCP_ESTABLISHED && c->state != TCP_CLOSE_WAIT &&
        c->state != TCP_FIN_WAIT_1 && c->state != TCP_LAST_ACK && c->state != TCP_CLOSING)
        return;
    while (!c->fin_sent) {
        uint32_t flight = c->snd_nxt - c->snd_una;      // sent, not yet acknowledged
        uint32_t unsent = c->snd_len - flight;
        if (!unsent)
            break;
        uint32_t room = c->snd_wnd > flight ? c->snd_wnd - flight : 0;
        if (!room) {
            arm_timer(c);                   // window closed: the timer will probe it
            break;
        }
        uint32_t n = unsent < room ? unsent : room;
        if (n > c->mss)
            n = c->mss;
        // Update our side FIRST, then send: on lo (and in an interrupt)
        // the answer can arrive before send returns, and its ACK must
        // already make sense to us.
        uint32_t seq = c->snd_nxt;
        c->snd_nxt += n;
        if (!c->rtt_seq) {                  // time this segment, if none is being timed
            c->rtt_seq = c->snd_nxt;
            c->rtt_start = net_now_ms();
        }
        arm_timer(c);
        tcp_send_segment(c, TCP_ACK | TCP_PSH, seq, c->sndbuf + flight, (uint16_t)n);
    }
    if (c->fin_wanted && !c->fin_sent && c->snd_nxt - c->snd_una == c->snd_len) {
        uint32_t seq = c->snd_nxt;
        c->snd_nxt++;                       // the FIN takes one sequence number
        c->fin_sent = 1;
        if (c->state == TCP_ESTABLISHED)
            tcp_set_state(c, TCP_FIN_WAIT_1);
        else if (c->state == TCP_CLOSE_WAIT)
            tcp_set_state(c, TCP_LAST_ACK);
        arm_timer(c);
        tcp_send_segment(c, TCP_FIN | TCP_ACK, seq, 0, 0);
    }
}

/* ---------- Using a connection (sock_tcp.c) ---------- */

static int port_conflict(uint32_t ip, uint16_t port, int reuse, const tcp_conn_t *self) {
    for (int i = 0; i < TCP_MAX_CONNS; i++) {
        const tcp_conn_t *o = &conns[i];
        if (!o->used || o == self || o->local_port != port)
            continue;
        if (ip && o->local_ip && ip != o->local_ip)
            continue;                       // different addresses of ours
        if (reuse && o->state != TCP_LISTEN && o->state != TCP_CLOSED)
            continue;                       // SO_REUSEADDR: an old connection may linger
        return 1;
    }
    return 0;
}

int tcp_bind(tcp_conn_t *c, uint32_t ip, uint16_t port, int reuse) {
    if (!port) {
        for (int tries = EPHEMERAL_HIGH - EPHEMERAL_LOW; tries > 0; tries--) {
            port = next_port;
            next_port = next_port >= EPHEMERAL_HIGH ? EPHEMERAL_LOW : next_port + 1;
            if (!port_conflict(ip, port, 0, c))
                break;
            port = 0;
        }
        if (!port)
            return -NET_EADDRINUSE;
    } else if (port_conflict(ip, port, reuse, c)) {
        return -NET_EADDRINUSE;
    }
    c->local_ip = ip;
    c->local_port = port;
    return 0;
}

int tcp_listen(tcp_conn_t *c, int backlog) {
    if (c->state != TCP_CLOSED && c->state != TCP_LISTEN)
        return -NET_EINVAL;
    if (!c->local_port) {
        int r = tcp_bind(c, 0, 0, 0);
        if (r < 0)
            return r;
    }
    c->backlog = backlog < 1 ? 1 : backlog > TCP_MAX_BACKLOG ? TCP_MAX_BACKLOG : backlog;
    tcp_set_state(c, TCP_LISTEN);
    return 0;
}

int tcp_connect(tcp_conn_t *c, uint32_t ip, uint16_t port) {
    if (c->state != TCP_CLOSED)
        return c->state == TCP_SYN_SENT ? -NET_EALREADY : -NET_EISCONN;
    route_result_t route;
    if (!route_lookup(ip, &route))
        return -NET_ENETUNREACH;
    if (!c->local_port) {
        int r = tcp_bind(c, 0, 0, 0);
        if (r < 0)
            return r;
    }
    if (!c->local_ip)
        c->local_ip = route.src;
    c->remote_ip = ip;
    c->remote_port = port;
    c->iss = new_isn(c);
    c->snd_una = c->iss;
    c->snd_nxt = c->iss + 1;                // the SYN takes one sequence number
    tcp_set_state(c, TCP_SYN_SENT);
    stats.active_opens++;
    tcp_send_segment(c, TCP_SYN, c->iss, 0, 0);
    arm_timer(c);
    return -NET_EINPROGRESS;
}

tcp_conn_t *tcp_accept(tcp_conn_t *listener) {
    for (int i = 0; i < TCP_MAX_CONNS; i++) {
        tcp_conn_t *c = &conns[i];
        if (c->used && c->listener == listener && !c->sock &&
            (c->state == TCP_ESTABLISHED || c->state == TCP_CLOSE_WAIT)) {
            listener->pending--;
            c->listener = 0;                // it belongs to its own socket now
            return c;
        }
    }
    return 0;
}

int tcp_ready_children(const tcp_conn_t *listener) {
    int n = 0;
    for (int i = 0; i < TCP_MAX_CONNS; i++) {
        const tcp_conn_t *c = &conns[i];
        n += c->used && c->listener == listener && !c->sock &&
             (c->state == TCP_ESTABLISHED || c->state == TCP_CLOSE_WAIT);
    }
    return n;
}

int tcp_send(tcp_conn_t *c, const void *buf, uint32_t len) {
    if (c->state == TCP_SYN_SENT || c->state == TCP_SYN_RECEIVED)
        return -NET_EAGAIN;                 // wait until it is connected
    if (c->state != TCP_ESTABLISHED && c->state != TCP_CLOSE_WAIT)
        return c->error ? -c->error : (c->state == TCP_CLOSED ? -NET_ENOTCONN : -NET_EPIPE);
    if (c->fin_wanted)
        return -NET_EPIPE;                  // we said we were done
    uint32_t room = TCP_BUF_SIZE - c->snd_len;
    if (!room)
        return -NET_EAGAIN;                 // the buffer is full: wait for ACKs
    uint32_t n = len < room ? len : room;
    memcpy(c->sndbuf + c->snd_len, buf, n);
    c->snd_len += n;
    tcp_output(c);
    return (int)n;
}

int tcp_recv(tcp_conn_t *c, void *buf, uint32_t len, int peek) {
    if (!c->rcv_len) {
        if (c->fin_received)
            return 0;                       // end of the stream
        if (c->state == TCP_CLOSED)
            return c->error ? -c->error : 0;
        if (c->state == TCP_LISTEN)
            return -NET_ENOTCONN;
        return -NET_EAGAIN;
    }
    uint16_t window_before = receive_window(c);
    uint32_t n = len < c->rcv_len ? len : c->rcv_len;
    uint8_t *out = buf;
    for (uint32_t i = 0; i < n; i++)
        out[i] = c->rcvbuf[(c->rcv_head + i) % TCP_BUF_SIZE];
    if (!peek) {
        c->rcv_head = (c->rcv_head + n) % TCP_BUF_SIZE;
        c->rcv_len -= n;
        // The window was (almost) shut: tell the peer there is room again.
        if (window_before < c->mss && receive_window(c) >= c->mss &&
            (c->state == TCP_ESTABLISHED || c->state == TCP_FIN_WAIT_1 || c->state == TCP_FIN_WAIT_2))
            send_ack(c);
    }
    return (int)n;
}

void tcp_shutdown_write(tcp_conn_t *c) {
    if (c->state == TCP_ESTABLISHED || c->state == TCP_CLOSE_WAIT) {
        c->fin_wanted = 1;
        tcp_output(c);
    }
}

void tcp_close(tcp_conn_t *c) {
    c->sock = 0;
    switch (c->state) {
    case TCP_LISTEN:
        // Connections nobody accepted are aborted with it.
        for (int i = 0; i < TCP_MAX_CONNS; i++) {
            tcp_conn_t *child = &conns[i];
            if (child->used && child->listener == c) {
                tcp_send_segment(child, TCP_RST | TCP_ACK, child->snd_nxt, 0, 0);
                child->listener = 0;
                tcp_free(child);
            }
        }
        tcp_free(c);
        break;
    case TCP_CLOSED:
    case TCP_SYN_SENT:
        tcp_free(c);
        break;
    case TCP_SYN_RECEIVED:
    case TCP_ESTABLISHED:
    case TCP_CLOSE_WAIT:
        if (c->rcv_len) {
            // Data the program never read: abort with RST (RFC 2525),
            // so the peer knows it was not delivered.
            tcp_send_segment(c, TCP_RST | TCP_ACK, c->snd_nxt, 0, 0);
            tcp_free(c);
        } else {
            c->fin_wanted = 1;              // lives on as an orphan: FIN, then TIME_WAIT
            tcp_output(c);
        }
        break;
    default:
        break;                              // already closing: the timers finish it
    }
}

int tcp_readable(const tcp_conn_t *c) {
    return c->rcv_len || c->fin_received || c->state == TCP_CLOSED;
}

int tcp_writable(const tcp_conn_t *c) {
    return (c->state == TCP_ESTABLISHED || c->state == TCP_CLOSE_WAIT) &&
           !c->fin_wanted && c->snd_len < TCP_BUF_SIZE;
}

/* ---------- Receiving ---------- */

// The MSS option of a SYN, or 0.
static uint16_t read_mss(const tcp_header_t *h, uint16_t hlen) {
    const uint8_t *o = (const uint8_t *)(h + 1), *end = (const uint8_t *)h + hlen;
    while (o < end && *o != OPT_END) {
        if (*o == OPT_NOP) {
            o++;
            continue;
        }
        if (o + 1 >= end || o[1] < 2 || o + o[1] > end)
            break;
        if (o[0] == OPT_MSS && o[1] == 4)
            return (uint16_t)((o[2] << 8) | o[3]);
        o += o[1];
    }
    return 0;
}

static tcp_conn_t *lookup(uint32_t dst, uint16_t dport, uint32_t src, uint16_t sport) {
    tcp_conn_t *listener = 0;
    for (int i = 0; i < TCP_MAX_CONNS; i++) {
        tcp_conn_t *c = &conns[i];
        if (!c->used || c->local_port != dport)
            continue;
        if (c->state == TCP_LISTEN) {
            if (!c->local_ip || c->local_ip == dst)
                listener = c;
        } else if (c->state != TCP_CLOSED && c->remote_port == sport &&
                   c->remote_ip == src && c->local_ip == dst) {
            return c;                       // the exact connection beats the listener
        }
    }
    return listener;
}

// No connection for this segment: answer with RST, so the sender stops
// ("connection refused" for a SYN to a closed port).
static void refuse(uint32_t dst, uint32_t src, const tcp_header_t *h, uint32_t seglen) {
    if (h->flags & TCP_RST)
        return;                             // never answer a RST
    if (!route_is_local(src) && !def_scan_rst_ok(src))
        return;                             // DEFENCE scan_detect: a scanner gets silence
    if (h->flags & TCP_ACK)
        send_raw(dst, src, ntohs(h->dst_port), ntohs(h->src_port), ntohl(h->ack), 0,
                 TCP_RST, 0, 0, 0, 0);
    else
        send_raw(dst, src, ntohs(h->dst_port), ntohs(h->src_port), 0, ntohl(h->seq) + seglen,
                 TCP_RST | TCP_ACK, 0, 0, 0, 0);
}

// A SYN for a listening socket: a new connection in SYN_RECEIVED.
static void new_child(tcp_conn_t *l, uint32_t dst, uint32_t src, const tcp_header_t *h, uint16_t hlen) {
    uint16_t mss = read_mss(h, hlen);
    if (l->pending >= l->backlog) {
        // The queue is full (a SYN flood fills it). DEFENCE syn_cookies:
        // remember nothing; the SYN-ACK's sequence number IS the memory.
        stats.backlog_drops++;
        uint32_t cookie = def_syn_cookie_make(dst, l->local_port, src, ntohs(h->src_port), ntohl(h->seq),
                                              def_mss_clamp(mss ? mss : TCP_DEFAULT_MSS));
        send_raw(dst, src, l->local_port, ntohs(h->src_port), cookie, ntohl(h->seq) + 1,
                 TCP_SYN | TCP_ACK, TCP_BUF_SIZE, 1, 0, 0);
        return;
    }
    tcp_conn_t *c = tcp_new(l->uid);
    if (!c) {
        stats.backlog_drops++;
        return;
    }
    c->local_ip = dst;
    c->local_port = l->local_port;
    c->remote_ip = src;
    c->remote_port = ntohs(h->src_port);
    c->listener = l;
    l->pending++;
    c->irs = ntohl(h->seq);
    c->rcv_nxt = c->irs + 1;
    c->iss = new_isn(c);
    c->snd_una = c->iss;
    c->snd_nxt = c->iss + 1;
    c->snd_wnd = ntohs(h->window);
    if (mss)                                // DEFENCE mss_min: never absurdly small
        c->mss = def_mss_clamp(mss) < OUR_MSS ? def_mss_clamp(mss) : OUR_MSS;
    tcp_set_state(c, TCP_SYN_RECEIVED);
    stats.passive_opens++;
    tcp_send_segment(c, TCP_SYN | TCP_ACK, c->iss, 0, 0);
    arm_timer(c);
}

// A new round-trip sample (Jacobson / Karels, RFC 6298).
static void measure(tcp_conn_t *c, uint32_t rtt) {
    if (!c->srtt_ms) {
        c->srtt_ms = rtt;
        c->rttvar_ms = rtt / 2;
    } else {
        uint32_t diff = c->srtt_ms > rtt ? c->srtt_ms - rtt : rtt - c->srtt_ms;
        c->rttvar_ms = (3 * c->rttvar_ms + diff) / 4;
        c->srtt_ms = (7 * c->srtt_ms + rtt) / 8;
    }
    uint32_t rto = c->srtt_ms + 4 * c->rttvar_ms;
    c->rto_ms = rto < TCP_RTO_MIN ? TCP_RTO_MIN : rto > TCP_RTO_MAX ? TCP_RTO_MAX : rto;
}

// The peer acknowledged everything before 'ack'.
static void take_ack(tcp_conn_t *c, uint32_t ack, uint16_t window) {
    if (SEQ_GT(ack, c->snd_una) && SEQ_LEQ(ack, c->snd_nxt)) {
        uint32_t acked = ack - c->snd_una;
        int fin_acked = c->fin_sent && ack == c->snd_nxt;
        uint32_t data = acked - (fin_acked ? 1 : 0);
        if (data > c->snd_len)
            data = c->snd_len;
        for (uint32_t i = data; i < c->snd_len; i++)     // drop the acknowledged bytes
            c->sndbuf[i - data] = c->sndbuf[i];
        c->snd_len -= data;
        c->snd_una = ack;
        if (c->rtt_seq && SEQ_GEQ(ack, c->rtt_seq)) {
            measure(c, net_now_ms() - c->rtt_start);
            c->rtt_seq = 0;
        }
        c->retries = 0;
        c->rtx_at = c->snd_una == c->snd_nxt ? 0 : net_now_ms() + c->rto_ms;
        if (fin_acked) {
            if (c->state == TCP_FIN_WAIT_1)
                tcp_set_state(c, TCP_FIN_WAIT_2);
            else if (c->state == TCP_CLOSING)
                tcp_set_state(c, TCP_TIME_WAIT);
            else if (c->state == TCP_LAST_ACK)
                tcp_finish(c, 0);
        }
        wake(c);                            // room in the send buffer
    }
    if (SEQ_GEQ(ack, c->snd_una)) {
        c->snd_wnd = window;
        if (window)
            c->probes = 0;                  // the window opened: no more probing
    }
}

// Copy in-order data into the receive ring. Returns 1 if the segment's
// end was reached (so a FIN on it may be taken too).
static int take_data(tcp_conn_t *c, uint32_t seq, const uint8_t *data, uint32_t len) {
    if (SEQ_LT(seq, c->rcv_nxt)) {          // starts with bytes we already have
        uint32_t old = c->rcv_nxt - seq;
        if (old >= len)
            return old == len;
        data += old;
        len -= old;
        seq = c->rcv_nxt;
    }
    if (seq != c->rcv_nxt)
        return 0;                           // a gap before it: drop, it will come again
    uint32_t room = TCP_BUF_SIZE - c->rcv_len;
    uint32_t n = len < room ? len : room;
    for (uint32_t i = 0; i < n; i++)
        c->rcvbuf[(c->rcv_head + c->rcv_len + i) % TCP_BUF_SIZE] = data[i];
    c->rcv_len += n;
    c->rcv_nxt += n;
    if (n)
        wake(c);
    return n == len;
}

static void syn_sent(tcp_conn_t *c, const tcp_header_t *h, uint16_t hlen) {
    uint32_t seq = ntohl(h->seq), ack = ntohl(h->ack);
    int ack_ok = (h->flags & TCP_ACK) && ack == c->iss + 1;
    if ((h->flags & TCP_ACK) && !ack_ok) {
        if (!(h->flags & TCP_RST))
            send_raw(c->local_ip, c->remote_ip, c->local_port, c->remote_port, ack, 0, TCP_RST, 0, 0, 0, 0);
        return;
    }
    if (h->flags & TCP_RST) {
        if (ack_ok) {
            stats.attempt_fails++;
            tcp_finish(c, NET_ECONNREFUSED);    // "nobody listens there"
        }
        return;
    }
    if (!(h->flags & TCP_SYN))
        return;
    c->irs = seq;
    c->rcv_nxt = seq + 1;
    uint16_t mss = read_mss(h, hlen);
    if (mss)                                // DEFENCE mss_min
        c->mss = def_mss_clamp(mss) < OUR_MSS ? def_mss_clamp(mss) : OUR_MSS;
    c->snd_wnd = ntohs(h->window);
    if (ack_ok) {
        c->snd_una = ack;
        c->rtx_at = 0;
        c->retries = 0;
        tcp_set_state(c, TCP_ESTABLISHED);      // the handshake's third step:
        send_ack(c);                        // ACK their SYN
        wake(c);
    } else {
        tcp_set_state(c, TCP_SYN_RECEIVED);     // both sides opened at once
        tcp_send_segment(c, TCP_SYN | TCP_ACK, c->iss, 0, 0);
    }
}

// A segment for a synchronized connection (every state after the handshake).
static void segment(tcp_conn_t *c, const tcp_header_t *h, const uint8_t *data, uint32_t len) {
    uint32_t seq = ntohl(h->seq), ack = ntohl(h->ack);
    uint32_t seglen = len + ((h->flags & TCP_SYN) ? 1 : 0) + ((h->flags & TCP_FIN) ? 1 : 0);

    // 1. In our window (DEFENCE seq_window_check)? If not, say where we
    //    are (unless it is a RST).
    if (!def_seq_in_window(seq, seglen, c->rcv_nxt, receive_window(c))) {
        if (!(h->flags & TCP_RST))
            challenge_ack(c);
        return;
    }
    // 2. RST: only at exactly the next expected byte; elsewhere in the
    //    window it gets a challenge ACK (RFC 5961, security/rst_injection).
    if (h->flags & TCP_RST) {
        if (c->state == TCP_TIME_WAIT && !def_timewait_rst_ok())
            return;                         // DEFENCE rfc1337: TIME_WAIT runs its full time
        if (def_rst_exact_ok(seq, c->rcv_nxt)) {    // DEFENCE rst_window_check
            if (c->state == TCP_SYN_RECEIVED && c->listener)
                tcp_free(c);               // never accepted: just forget it
            else
                tcp_finish(c, c->state == TCP_SYN_RECEIVED ? NET_ECONNREFUSED : NET_ECONNRESET);
            stats.estab_resets++;
        } else {
            challenge_ack(c);
        }
        return;
    }
    // 3. A SYN in an open connection: challenge ACK too.
    if (h->flags & TCP_SYN) {
        challenge_ack(c);
        return;
    }
    // 4. From here on every segment must carry an ACK.
    if (!(h->flags & TCP_ACK))
        return;
    if (c->state == TCP_SYN_RECEIVED) {
        if (ack != c->iss + 1) {
            send_raw(c->local_ip, c->remote_ip, c->local_port, c->remote_port, ack, 0, TCP_RST, 0, 0, 0, 0);
            return;
        }
        c->snd_una = ack;
        c->rtx_at = 0;
        c->retries = 0;
        tcp_set_state(c, TCP_ESTABLISHED);      // ready for accept()
        wake(c);
    }
    take_ack(c, ack, ntohs(h->window));
    if (c->state == TCP_CLOSED || !c->used)
        return;                             // LAST_ACK's ACK: it is over

    // 5. Data for a connection the program already closed (an orphan in
    //    FIN_WAIT, CLOSING, LAST_ACK): nobody will ever read it. Answer
    //    with RST, as RFC 1122 asks, and forget the connection.
    if (len && !c->sock && !c->listener && c->state != TCP_ESTABLISHED && c->state != TCP_CLOSE_WAIT) {
        tcp_send_segment(c, TCP_RST | TCP_ACK, c->snd_nxt, 0, 0);
        tcp_free(c);
        return;
    }

    // 6. Data, then the FIN, both only when they are next in line.
    int need_ack = 0, whole = 1;
    if (len && (c->state == TCP_ESTABLISHED || c->state == TCP_FIN_WAIT_1 || c->state == TCP_FIN_WAIT_2)) {
        whole = take_data(c, seq, data, len);
        need_ack = 1;
    }
    if ((h->flags & TCP_FIN) && whole && seq + len == c->rcv_nxt && !c->fin_received) {
        c->rcv_nxt++;                       // the FIN takes one sequence number
        c->fin_received = 1;
        need_ack = 1;
        if (c->state == TCP_ESTABLISHED || c->state == TCP_SYN_RECEIVED)
            tcp_set_state(c, TCP_CLOSE_WAIT);   // they are done; we may still send
        else if (c->state == TCP_FIN_WAIT_1)
            tcp_set_state(c, TCP_CLOSING);      // both sent FIN at the same time
        else if (c->state == TCP_FIN_WAIT_2)
            tcp_set_state(c, TCP_TIME_WAIT);
        wake(c);
    }
    if (need_ack)
        send_ack(c);
    tcp_output(c);                          // the window may have opened
}

// An ACK to a listener for no connection: if its ack - 1 is a SYN cookie
// we made, the client is real. Make the connection, ESTABLISHED at once,
// and let segment() take whatever data came with the ACK. 1 = done.
static int cookie_child(tcp_conn_t *l, uint32_t dst, uint32_t src, const tcp_header_t *h,
                        const uint8_t *data, uint32_t len) {
    uint32_t seq = ntohl(h->seq), ack = ntohl(h->ack);
    uint16_t mss = def_syn_cookie_check(dst, l->local_port, src, ntohs(h->src_port), seq - 1, ack - 1);
    tcp_conn_t *c = mss ? tcp_new(l->uid) : 0;
    if (!c)
        return 0;
    c->local_ip = dst;
    c->local_port = l->local_port;
    c->remote_ip = src;
    c->remote_port = ntohs(h->src_port);
    c->listener = l;
    l->pending++;
    c->irs = seq - 1;
    c->rcv_nxt = seq;
    c->iss = ack - 1;
    c->snd_una = c->snd_nxt = ack;
    c->snd_wnd = ntohs(h->window);
    c->mss = mss < OUR_MSS ? mss : OUR_MSS;
    tcp_set_state(c, TCP_ESTABLISHED);
    stats.passive_opens++;
    wake(c);                                // the listener has one to accept
    if (len || (h->flags & TCP_FIN))
        segment(c, h, data, len);
    return 1;
}

void tcp_icmp_error(const uint8_t *quote, uint16_t len, int err) {
    const ipv4_header_t *ip = (const ipv4_header_t *)quote;
    uint16_t hlen = (uint16_t)((ip->version_ihl & 0x0F) * 4);
    if (len < hlen + 8)
        return;
    const tcp_header_t *h = (const tcp_header_t *)(quote + hlen);
    // It quotes OUR segment: its source is our end.
    tcp_conn_t *c = lookup(ntohl(ip->src), ntohs(h->src_port), ntohl(ip->dst), ntohs(h->dst_port));
    if (!c || c->state != TCP_SYN_SENT || ntohl(h->seq) != c->iss)
        return;                             // only a connect() in progress gives up
    stats.attempt_fails++;
    tcp_finish(c, -err);
}

void tcp_receive(packet_t *p) {
    stats.in_segs++;
    ipv4_header_t *ip = ipv4_header(p);
    uint32_t src = ntohl(ip->src), dst = ntohl(ip->dst);
    tcp_header_t *h = (tcp_header_t *)p->data;
    uint16_t hlen = p->len >= TCP_HLEN ? (uint16_t)((h->offset >> 4) * 4) : 0;
    if (hlen < TCP_HLEN || hlen > p->len ||
        inet_checksum_pseudo(src, dst, IPPROTO_TCP, p->data, p->len) != 0) {
        stats.in_errs++;                    // short, or damaged
        packet_free(p);
        return;
    }
    if (!def_tcp_flags_ok(h->flags)) {      // DEFENCE tcp_flag_sanity: scans get nothing
        stats.in_errs++;
        packet_free(p);
        return;
    }
    uint32_t len = p->len - hlen;
    const uint8_t *data = p->data + hlen;
    uint32_t seglen = len + ((h->flags & TCP_SYN) ? 1 : 0) + ((h->flags & TCP_FIN) ? 1 : 0);

    tcp_conn_t *c = lookup(dst, ntohs(h->dst_port), src, ntohs(h->src_port));
    if (!c) {
        refuse(dst, src, h, seglen);
    } else {
        c->segs_in++;
        if (c->state == TCP_LISTEN) {
            if (h->flags & TCP_RST)
                ;                           // nothing to reset
            else if (h->flags & TCP_ACK) {
                // An ACK for a connection we never had: the answer to a
                // SYN cookie, or a stray. DEFENCE syn_cookies checks it.
                if (!cookie_child(c, dst, src, h, data, len))
                    refuse(dst, src, h, seglen);
            }
            else if (h->flags & TCP_SYN)
                new_child(c, dst, src, h, hlen);
        } else if (c->state == TCP_SYN_SENT) {
            syn_sent(c, h, hlen);
        } else {
            segment(c, h, data, len);
        }
    }
    packet_free(p);
}

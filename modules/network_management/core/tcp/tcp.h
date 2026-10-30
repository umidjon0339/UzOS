#ifndef TCP_H
#define TCP_H

/*
 * tcp.h - TCP: a reliable, ordered stream of bytes between two programs.
 *
 *   0               16              31
 *   +---------------+---------------+
 *   |  source port  |  dest port    |
 *   +---------------+---------------+
 *   |        sequence number        |   the number of our first data byte here
 *   +-------------------------------+
 *   |     acknowledgment number     |   "I have every byte before this one"
 *   +----+------+---+---------------+
 *   |offs| rsvd |flg|    window     |   window: how much more I can take
 *   +----+------+---+---------------+
 *   |   checksum    |  urgent ptr   |
 *   +---------------+---------------+
 *   |  options (MSS ...)  |  data   |
 *
 *   flags: SYN (start), ACK, FIN (I am done), RST (abort), PSH
 *
 * Every byte has a number. The receiver acknowledges what it got; what
 * is not acknowledged in time is sent again. That is all "reliable"
 * means: number, acknowledge, retransmit.
 *
 * A connection begins with the three-way handshake:
 *
 *   client  SYN seq=x                 ->  server   (SYN_SENT)
 *           <- SYN+ACK seq=y ack=x+1      server   (SYN_RECEIVED)
 *   client  ACK ack=y+1               ->  both     (ESTABLISHED)
 *
 * and ends with a FIN from each side (each acknowledged). Whoever sent
 * the first FIN waits in TIME_WAIT a while, in case the last ACK was lost.
 *
 * tcp.c handles segments (the state machine), tcp_timer.c the clocks.
 * Sockets use a connection through sock_tcp.c.
 */

#include <stdint.h>
#include "modules/network_management/core/packet/packet.h"     // packet_t

#define TCP_HLEN          20
#define TCP_MAX_CONNS     24
#define TCP_BUF_SIZE      8192        // bytes buffered each way, per connection
#define TCP_DEFAULT_MSS   536         // RFC 879: what we assume without the option
#define TCP_MAX_BACKLOG   16

// Header flags
#define TCP_FIN 0x01
#define TCP_SYN 0x02
#define TCP_RST 0x04
#define TCP_PSH 0x08
#define TCP_ACK 0x10
#define TCP_URG 0x20

// Timers (milliseconds)
#define TCP_RTO_INITIAL   1000        // retransmit after this, before we have measured
#define TCP_RTO_MIN       200
#define TCP_RTO_MAX       30000
#define TCP_SYN_RETRIES   5
#define TCP_DATA_RETRIES  8
#define TCP_TIME_WAIT_MS  10000       // 2 x MSL (Linux: 60 s)
#define TCP_FIN_WAIT2_MS  30000       // a peer that never sends its FIN

// Sequence numbers wrap around at 2^32: compare them by their difference.
#define SEQ_LT(a, b)  ((int32_t)((a) - (b)) < 0)
#define SEQ_LEQ(a, b) ((int32_t)((a) - (b)) <= 0)
#define SEQ_GT(a, b)  ((int32_t)((a) - (b)) > 0)
#define SEQ_GEQ(a, b) ((int32_t)((a) - (b)) >= 0)

typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t seq;
    uint32_t ack;
    uint8_t  offset;                  // header length in 4-byte words, in the high 4 bits
    uint8_t  flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent;
} __attribute__((packed)) tcp_header_t;

typedef enum {
    TCP_CLOSED, TCP_LISTEN, TCP_SYN_SENT, TCP_SYN_RECEIVED, TCP_ESTABLISHED,
    TCP_FIN_WAIT_1, TCP_FIN_WAIT_2, TCP_CLOSE_WAIT, TCP_CLOSING, TCP_LAST_ACK,
    TCP_TIME_WAIT,
} tcp_state_t;

struct socket;

typedef struct tcp_conn {
    int used;
    tcp_state_t state;
    uint32_t local_ip, remote_ip;     // host order
    uint16_t local_port, remote_port;
    struct socket *sock;              // the socket using it (NULL: not accepted yet, or closed)
    uint32_t uid;                     // whose it is

    // Sending. sndbuf holds the bytes from snd_una on: the first
    // (snd_nxt - snd_una) are sent but not yet acknowledged, the rest unsent.
    uint32_t iss, snd_una, snd_nxt;   // first seq, oldest unacked, next to send
    uint32_t snd_wnd;                 // the peer's window: how much it can take
    uint16_t mss;                     // biggest segment the peer accepts
    uint8_t  sndbuf[TCP_BUF_SIZE];
    uint32_t snd_len;
    int fin_wanted;                   // close / shutdown(WR): FIN after the data
    int fin_sent;

    // Receiving: a ring of bytes the program has not read yet.
    uint32_t irs, rcv_nxt;            // the peer's first seq, the next byte we expect
    uint8_t  rcvbuf[TCP_BUF_SIZE];
    uint32_t rcv_head, rcv_len;
    int fin_received;                 // the peer said FIN: after the data, read() = 0

    // Timers
    uint32_t rto_ms;                  // the retransmission timeout now
    uint32_t srtt_ms, rttvar_ms;      // the measured round-trip time
    uint32_t rtt_seq, rtt_start;      // the segment being timed (0 = none)
    uint32_t rtx_at;                  // retransmit when the clock passes this (0 = off)
    int      retries;
    int      probes;                  // window probes while the peer's window is 0
    uint32_t state_since;             // when the current state began (TIME_WAIT, ...)

    // A listening socket's children: connections being made or ready.
    struct tcp_conn *listener;        // the listener this one came from
    int backlog;                      // listener: how many may wait
    int pending;                      // listener: children not accepted yet

    uint32_t challenge_since;         // challenge ACKs sent since then ...
    int challenges;                   // ... (a few per tick at most)

    int error;                        // why it ended: NET_ECONNREFUSED, ...
    uint32_t segs_in, segs_out, retransmits;
} tcp_conn_t;

typedef struct {
    uint32_t active_opens, passive_opens, attempt_fails, estab_resets;
    uint32_t in_segs, out_segs, retrans_segs, in_errs, out_rsts;
    uint32_t backlog_drops;
} tcp_stats_t;

/* ---------- For sockets (sock_tcp.c) ---------- */

tcp_conn_t *tcp_new(uint32_t uid);
int  tcp_bind(tcp_conn_t *c, uint32_t ip, uint16_t port, int reuse);  // 0 or -NET_EADDRINUSE
int  tcp_listen(tcp_conn_t *c, int backlog);
int  tcp_connect(tcp_conn_t *c, uint32_t ip, uint16_t port);         // -NET_EINPROGRESS
tcp_conn_t *tcp_accept(tcp_conn_t *listener);                         // NULL: none ready
int  tcp_ready_children(const tcp_conn_t *listener);                  // how many accept() would get
int  tcp_send(tcp_conn_t *c, const void *buf, uint32_t len);          // bytes taken, or -NET_E*
int  tcp_recv(tcp_conn_t *c, void *buf, uint32_t len, int peek);      // bytes, 0 = EOF, or -NET_E*
void tcp_shutdown_write(tcp_conn_t *c);
// The program is done with it. It may still live on (FIN, TIME_WAIT).
void tcp_close(tcp_conn_t *c);
int  tcp_readable(const tcp_conn_t *c);
int  tcp_writable(const tcp_conn_t *c);

/* ---------- For the stack ---------- */

// A received TCP segment (IP header pulled). Takes p.
void tcp_receive(packet_t *p);

// Retransmissions and state timeouts (net_tick).
void tcp_tick(uint32_t now_ms);

// ICMP says a segment of ours could not be delivered ('quote' = our IP
// header + 8 bytes: the ports and seq). A connection still being made
// gives up at once (ECONNREFUSED / EHOSTUNREACH), as in Linux.
void tcp_icmp_error(const uint8_t *quote, uint16_t len, int err);

/* ---------- Inside TCP: shared by tcp.c and tcp_timer.c ---------- */

tcp_conn_t *tcp_slot(int slot);                     // 0 .. TCP_MAX_CONNS-1, used or not
void tcp_set_state(tcp_conn_t *c, tcp_state_t state);
void tcp_output(tcp_conn_t *c);                     // send what the window allows, then FIN
int  tcp_send_segment(tcp_conn_t *c, uint8_t flags, uint32_t seq, const uint8_t *data, uint16_t len);
void tcp_finish(tcp_conn_t *c, int error);          // over: CLOSED (freed if no socket holds it)
void tcp_free(tcp_conn_t *c);

/* ---------- For netstat / ss ---------- */

const tcp_conn_t *tcp_conn(int slot);
const char *tcp_state_name(tcp_state_t state);
tcp_stats_t *tcp_stats(void);

#endif

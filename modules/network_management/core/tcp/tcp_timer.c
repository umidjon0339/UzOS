/*
 * tcp_timer.c - TCP's clocks: retransmit, probe a shut window, give up.
 *
 * Every NET_TICK_MS (net_tick) each connection is checked:
 *
 *   retransmit   nothing acknowledged for rto_ms: send again from the
 *                oldest unacknowledged byte (or the SYN), and double
 *                rto_ms ("exponential backoff"); after too many tries
 *                the connection dies with ETIMEDOUT
 *   window probe the peer's window is 0 and we have data: send one byte
 *                now and then, so its reopening is never missed
 *   TIME_WAIT    wait TCP_TIME_WAIT_MS, then free the connection
 *   FIN_WAIT_2   an orphan whose peer never sends its FIN: free it
 *
 * Uses: tcp_* (tcp.c), def_persist_ok (security/zero_window/defence/persist_timeout).
 */

#include "modules/network_management/core/tcp/tcp.h"             // tcp_conn_t, tcp_*
#include "modules/network_management/core/inet/inet.h"           // NET_ETIMEDOUT
#include "modules/network_management/security/zero_window/defence/persist_timeout/def_persist_timeout.h"  // def_persist_ok

static void retransmit(tcp_conn_t *c, uint32_t now_ms) {
    int limit = (c->state == TCP_SYN_SENT || c->state == TCP_SYN_RECEIVED)
                ? TCP_SYN_RETRIES : TCP_DATA_RETRIES;
    if (++c->retries > limit) {
        tcp_stats()->attempt_fails += c->state == TCP_SYN_SENT;
        tcp_finish(c, NET_ETIMEDOUT);       // the peer is gone
        return;
    }
    c->rto_ms = c->rto_ms * 2 > TCP_RTO_MAX ? TCP_RTO_MAX : c->rto_ms * 2;
    c->rtx_at = now_ms + c->rto_ms;
    c->rtt_seq = 0;                         // Karn: never time a resent segment
    c->retransmits++;
    tcp_stats()->retrans_segs++;

    switch (c->state) {
    case TCP_SYN_SENT:
        tcp_send_segment(c, TCP_SYN, c->iss, 0, 0);
        return;
    case TCP_SYN_RECEIVED:
        tcp_send_segment(c, TCP_SYN | TCP_ACK, c->iss, 0, 0);
        return;
    default:
        break;
    }
    if (c->snd_una == c->snd_nxt && c->snd_len && c->snd_wnd == 0) {
        // Window probe: one byte beyond the shut window. The peer drops
        // it, but its ACK tells us the window again. DEFENCE
        // persist_timeout: a window that never opens ends the connection.
        if (!def_persist_ok(++c->probes)) {
            tcp_finish(c, NET_ETIMEDOUT);
            return;
        }
        tcp_send_segment(c, TCP_ACK, c->snd_nxt, c->sndbuf, 1);
        return;
    }
    // Go back to the oldest unacknowledged byte and send from there
    // (a FIN that was lost is sent again after the data).
    c->snd_nxt = c->snd_una;
    c->fin_sent = 0;
    tcp_output(c);
}

void tcp_tick(uint32_t now_ms) {
    for (int i = 0; i < TCP_MAX_CONNS; i++) {
        tcp_conn_t *c = tcp_slot(i);
        if (!c->used)
            continue;
        uint32_t in_state = now_ms - c->state_since;
        if (c->state == TCP_TIME_WAIT && in_state >= TCP_TIME_WAIT_MS) {
            if (c->sock)
                tcp_finish(c, 0);
            else
                tcp_free(c);
        } else if (c->state == TCP_FIN_WAIT_2 && !c->sock && in_state >= TCP_FIN_WAIT2_MS) {
            tcp_free(c);
        } else if (c->rtx_at && (int32_t)(now_ms - c->rtx_at) >= 0) {
            retransmit(c, now_ms);
        }
    }
}

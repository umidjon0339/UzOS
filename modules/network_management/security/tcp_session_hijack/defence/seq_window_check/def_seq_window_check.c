/*
 * def_seq_window_check.c - DEFENCE seq_window_check: the receive window test (RFC 793).
 *
 * Stops:  tcp_session_hijack (security/tcp_session_hijack)
 * Called: segment() (core/tcp), first, for every segment of an open connection.
 * Rule:   some byte of the segment must fall in [rcv_nxt, rcv_nxt + window);
 *         an empty one must start in it. Otherwise: a (limited) ACK, nothing else.
 * Why:    with unguessable ISNs, an attacker who cannot see the
 *         connection cannot hit the window, let alone the next byte.
 *
 * Uses: audit_log (security_management).
 */

#include "modules/network_management/security/tcp_session_hijack/defence/seq_window_check/def_seq_window_check.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                       // audit_log

static int in_window(uint32_t seq, uint32_t seglen, uint32_t rcv_nxt, uint32_t wnd) {
    int32_t first = (int32_t)(seq - rcv_nxt), last = (int32_t)(seq + seglen - 1 - rcv_nxt);
    if (seglen == 0)
        return wnd == 0 ? seq == rcv_nxt : first >= 0 && first < (int32_t)wnd;
    if (wnd == 0)
        return 0;
    return (first >= 0 && first < (int32_t)wnd) || (last >= 0 && last < (int32_t)wnd);
}

int def_seq_in_window(uint32_t seq, uint32_t seglen, uint32_t rcv_nxt, uint32_t wnd) {
    if (in_window(seq, seglen, rcv_nxt, wnd))
        return 1;
    audit_log("seq_window_check", "a TCP segment outside the receive window was refused");
    return 0;
}

static int selftest(void) {
    if (!in_window(1000, 10, 1000, 8192)) return 0;     // exactly next
    if (!in_window(995, 10, 1000, 8192)) return 0;      // overlaps the start
    if (in_window(21000, 6, 1000, 8192)) return 0;      // far ahead
    if (in_window(900, 10, 1000, 8192)) return 0;       // all old
    if (!in_window(1000, 0, 1000, 0)) return 0;         // a pure ACK, window shut
    return 1;
}

static const defence_t defence = {
    "seq_window_check",
    "a segment must lie in the receive window, and data must be next in line",
    "tcp_session_hijack",
    selftest
};

void def_seq_window_check_register(void) { registry_add_defence(&defence); }

/*
 * def_syn_cookies.c - DEFENCE syn_cookies: SYN cookies: no memory until the client proves it is real.
 *
 * Stops:  syn_flood (security/syn_flood)
 * Called: tcp.c: new_child() makes a cookie when the listen queue is full;
 *         a listener that gets an ACK for no connection checks it.
 * Rule:   the SYN-ACK's sequence number is a keyed hash of the four
 *         addresses and ports, the client's sequence number and the
 *         time (64 s steps), with the MSS in its low 3 bits. Nothing is
 *         stored. An ACK whose ack-1 is a valid cookie (this time step or
 *         the one before) becomes an ESTABLISHED connection at once.
 * Why:    forged sources never answer, so they cost nothing; a real
 *         client's ACK carries everything needed (D. J. Bernstein, 1996).
 *
 * Uses: tsc_now (cpu_management), net_now_ms (core/net_tick), audit_log (security_management).
 */

#include "modules/network_management/security/syn_flood/defence/syn_cookies/def_syn_cookies.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                       // audit_log
#include "modules/cpu_management/core/tsc/tsc.h"                                // tsc_now
#include "modules/network_management/core/net_tick/net_tick.h"                  // net_now_ms

static const uint16_t sizes[8] = { 536, 1024, 1220, 1300, 1400, 1440, 1460, 1460 };
static uint32_t secret;

// A keyed hash of everything that names the connection, and the time step.
static uint32_t hash(uint32_t a, uint16_t ap, uint32_t b, uint16_t bp, uint32_t seq, uint32_t step) {
    if (!secret)
        secret = (uint32_t)tsc_now() * 2246822519u | 1;
    uint32_t parts[5] = { a, b, ((uint32_t)ap << 16) | bp, seq, step };
    uint32_t h = secret;
    for (int i = 0; i < 5; i++) {
        h ^= parts[i];
        h *= 0x85EBCA6Bu;
        h ^= h >> 13;
    }
    return h;
}

static uint32_t make(uint32_t a, uint16_t ap, uint32_t b, uint16_t bp, uint32_t seq, uint32_t step, int mss_index) {
    return (hash(a, ap, b, bp, seq, step) & ~7u) | (uint32_t)mss_index;
}

static int index_of(uint16_t mss) {
    int i = 0;
    while (i < 6 && sizes[i + 1] <= mss)
        i++;
    return i;
}

uint32_t def_syn_cookie_make(uint32_t local_ip, uint16_t local_port, uint32_t remote_ip,
                             uint16_t remote_port, uint32_t their_seq, uint16_t mss) {
    uint32_t step = net_now_ms() / 64000;
    return make(local_ip, local_port, remote_ip, remote_port, their_seq, step, index_of(mss));
}

uint16_t def_syn_cookie_check(uint32_t local_ip, uint16_t local_port, uint32_t remote_ip,
                              uint16_t remote_port, uint32_t their_seq, uint32_t cookie) {
    uint32_t step = net_now_ms() / 64000;
    int mss_index = (int)(cookie & 7);
    for (uint32_t back = 0; back < 2; back++)
        if (make(local_ip, local_port, remote_ip, remote_port, their_seq, step - back, mss_index) == cookie)
            return sizes[mss_index];
    audit_log("syn_cookies", "an ACK with a forged or old SYN cookie was refused");
    return 0;
}

static int selftest(void) {
    uint32_t c = make(1, 80, 2, 4000, 777, 5, index_of(1460));
    if ((c & 7) != (uint32_t)index_of(1460)) return 0;
    if (make(1, 80, 2, 4000, 777, 5, index_of(1460)) != c) return 0;   // same input, same cookie
    if (make(1, 80, 2, 4001, 777, 5, index_of(1460)) == c) return 0;   // another port: another cookie
    if (make(1, 80, 2, 4000, 777, 6, index_of(1460)) == c) return 0;   // another time step
    return sizes[index_of(536)] == 536 && sizes[index_of(1500)] == 1460;
}

static const defence_t defence = {
    "syn_cookies",
    "a full listen queue answers with SYN cookies instead of dropping",
    "syn_flood",
    selftest
};

void def_syn_cookies_register(void) { registry_add_defence(&defence); }

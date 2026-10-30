/*
 * def_random_isn.c - DEFENCE random_isn: unguessable initial sequence numbers (RFC 6528).
 *
 * Stops:  tcp_seq_prediction (security/tcp_seq_prediction)
 * Called: tcp_connect() and new_child() (core/tcp), for every new connection.
 * Rule:   ISN = clock (250 per ms) + hash(secret, addresses, ports).
 * Why:    the clock keeps old segments of a reused connection out of the
 *         new one; the secret hash makes one connection say nothing
 *         about another (RFC 6528).
 *
 * Uses: tsc_now (cpu_management), net_now_ms (core/net_tick).
 */

#include "modules/network_management/security/tcp_seq_prediction/defence/random_isn/def_random_isn.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence
#include "modules/cpu_management/core/tsc/tsc.h"                                // tsc_now
#include "modules/network_management/core/net_tick/net_tick.h"                  // net_now_ms

static uint32_t secret;

static uint32_t hash(uint32_t key, uint32_t a, uint32_t b, uint32_t ports) {
    uint32_t parts[3] = { a, b, ports }, h = key;
    for (int i = 0; i < 3; i++) {
        h ^= parts[i];
        h *= 0x9E3779B1u;                   // multiply by an odd constant ...
        h ^= h >> 15;                       // ... and fold the high bits down
    }
    return h;
}

uint32_t def_isn(uint32_t local_ip, uint16_t local_port, uint32_t remote_ip, uint16_t remote_port) {
    if (!secret)
        secret = (uint32_t)tsc_now() * 2654435761u | 1;     // made once, at the first connection
    return hash(secret, local_ip, remote_ip, ((uint32_t)local_port << 16) | remote_port) + net_now_ms() * 250;
}

static int selftest(void) {
    uint32_t a = hash(99, 1, 2, 80), b = hash(99, 1, 2, 81);
    uint32_t step = b - a;
    return a != b && step > (1u << 20) && step < 0u - (1u << 20);
}

static const defence_t defence = {
    "random_isn",
    "every connection's ISN is a keyed hash of its addresses plus a clock",
    "tcp_seq_prediction",
    selftest
};

void def_random_isn_register(void) { registry_add_defence(&defence); }

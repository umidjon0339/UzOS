/*
 * def_random_ip_id.c - DEFENCE random_ip_id: random IP identification.
 *
 * Stops:  ip_id_prediction (security/ip_id_prediction)
 * Called: ipv4_send() (core/ipv4), for every packet.
 * Rule:   the id comes from a fast pseudo-random generator (xorshift)
 *         seeded from the CPU clock, not from a counter.
 * Why:    the id only has to tell fragments of one datagram apart; a
 *         counter leaks how busy we are (idle scan, RFC 6864).
 *
 * Uses: tsc_now (cpu_management).
 */

#include "modules/network_management/security/ip_id_prediction/defence/random_ip_id/def_random_ip_id.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence
#include "modules/cpu_management/core/tsc/tsc.h"                                // tsc_now

static uint32_t state;

// xorshift32: a fast generator; never 0 once seeded.
static uint32_t next(uint32_t x) {
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}

uint16_t def_next_ip_id(void) {
    if (!state)
        state = (uint32_t)tsc_now() | 1;
    state = next(state);
    return (uint16_t)(state >> 8);
}

static int selftest(void) {
    uint32_t x = 12345, steps = 0;
    for (int i = 0; i < 8; i++) {
        uint32_t y = next(x);
        steps += (uint16_t)((y >> 8) - (x >> 8)) == 1;
        x = y;
    }
    return steps == 0;                          // never "previous + 1" in a row
}

static const defence_t defence = {
    "random_ip_id",
    "every IP id is unpredictable",
    "ip_id_prediction",
    selftest
};

void def_random_ip_id_register(void) { registry_add_defence(&defence); }

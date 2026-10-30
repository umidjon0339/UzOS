/*
 * def_dns_txid_port_random.c - DEFENCE dns_txid_port_random: 32 random bits per DNS query.
 *
 * Stops:  dns_spoofing (security/dns_spoofing)
 * Called: dns_query() (core/dns), for every question it sends.
 * Rule:   the 16-bit id and the source port (49152-65535) are both
 *         random (xorshift seeded from the CPU clock); the answer must
 *         match both, the question, and come from the server asked.
 * Why:    RFC 5452, after Kaminsky's cache poisoning (2008).
 *
 * Uses: tsc_now (cpu_management).
 */

#include "modules/network_management/security/dns_spoofing/defence/dns_txid_port_random/def_dns_txid_port_random.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence
#include "modules/cpu_management/core/tsc/tsc.h"                                // tsc_now

static uint32_t state;

static uint32_t next(void) {
    if (!state)
        state = (uint32_t)tsc_now() | 1;
    state ^= state << 13;                   // xorshift32
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

static uint16_t port_from(uint32_t r) {
    return (uint16_t)(DNS_PORT_LOW + r % (DNS_PORT_HIGH - DNS_PORT_LOW + 1));
}

uint16_t def_dns_id(void)   { return (uint16_t)(next() >> 8); }
uint16_t def_dns_port(void) { return port_from(next()); }

static int selftest(void) {
    for (uint32_t r = 0; r < 70000; r += 997)
        if (port_from(r) < DNS_PORT_LOW)
            return 0;
    uint16_t a = def_dns_port(), b = def_dns_port();
    return a != b || def_dns_port() != a;
}

static const defence_t defence = {
    "dns_txid_port_random",
    "every DNS query uses a random id and a random source port",
    "dns_spoofing",
    selftest
};

void def_dns_txid_port_random_register(void) { registry_add_defence(&defence); }

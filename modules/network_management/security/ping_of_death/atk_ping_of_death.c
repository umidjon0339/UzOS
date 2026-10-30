/*
 * atk_ping_of_death.c - ATTACK ping_of_death: a fragment that ends beyond any datagram
 *
 * Breaks:     integrity - putting fragments together never writes past the buffer
 * Stopped by: reassembly_bounds (defence/reassembly_bounds)
 *
 * An IP datagram is at most 65535 bytes, but the LAST fragment's offset
 * (up to 8191 x 8 = 65528) plus its length can exceed that. Stacks that
 * copied it into a 64 KiB buffer overran it: the 1996 "ping of death"
 * crashed most systems of the time. UzOS's buffer is smaller still.
 *
 * simulate(): show the arithmetic.
 * launch():   inject a first fragment and one at offset 65528.
 *             findings = 1 if the far fragment was stored.
 * cleanup():  drop the half-built datagram.
 *
 * Uses: netdev_inject (core/netdev), ipv4_stats / ipv4_fragment_flush (core/ipv4), ui_*.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/ping_of_death/defence/reassembly_bounds/def_reassembly_bounds.h" // IPV4_REASM_MAX via ipv4.h
#include "modules/network_management/core/netdev/netdev.h"                      // netdev_by_name, netdev_inject, netdev_lock
#include "modules/network_management/core/ethernet/ethernet.h"                  // eth_header_t, ETH_*
#include "modules/network_management/core/ipv4/ipv4.h"                          // ipv4_header_t, ipv4_stats
#include "modules/network_management/core/icmp/icmp.h"                          // icmp_header_t, icmp_stats
#include "modules/network_management/core/inet/inet.h"                          // htons, inet_checksum, INET_ADDR
#include "lib/string/string.h"                                                  // memcpy, memset
#include "modules/ui/core/style/ui.h"                                           // ui_printf, ui_color

// An IPv4 packet inside an Ethernet frame for 'dev', as a machine on the
// link would send it: 'opt' (opt_len bytes of options), then 'data'.
// Returns the frame's length.
static int build_ip(uint8_t *f, const netdev_t *dev, uint32_t src, uint32_t dst, uint8_t proto,
                    uint16_t id, uint16_t frag, const uint8_t *opt, int opt_len, const void *data, int len) {
    static const uint8_t neighbour[6] = { 0x02, 0x00, 0x00, 0xaa, 0xbb, 0xcc };
    int hlen = IPV4_HLEN + opt_len;
    memset(f, 0, ETH_ZLEN);
    eth_header_t *eth = (eth_header_t *)f;
    memcpy(eth->dest, dev->mac, 6);
    memcpy(eth->src, neighbour, 6);
    eth->type = htons(ETH_P_IP);
    ipv4_header_t *ip = (ipv4_header_t *)(f + ETH_HLEN);
    ip->version_ihl = (uint8_t)(0x40 | (hlen / 4));
    ip->total_length = htons((uint16_t)(hlen + len));
    ip->id = htons(id);
    ip->frag = htons(frag);
    ip->ttl = 64;
    ip->protocol = proto;
    ip->src = htonl(src);
    ip->dst = htonl(dst);
    if (opt_len)
        memcpy(f + ETH_HLEN + IPV4_HLEN, opt, opt_len);
    ip->checksum = inet_checksum(ip, hlen);
    memcpy(f + ETH_HLEN + hlen, data, len);
    return ETH_HLEN + hlen + len < ETH_ZLEN ? ETH_ZLEN : ETH_HLEN + hlen + len;
}

// An ICMP echo request (8 bytes of header + 8 of data), checksum filled in.
static int build_echo(uint8_t *out, uint16_t id, uint16_t seq) {
    icmp_header_t *h = (icmp_header_t *)out;
    memset(out, 0, 16);
    h->type = ICMP_ECHO_REQUEST;
    h->id = htons(id);
    h->seq = htons(seq);
    memcpy(out + ICMP_HLEN, "seclab!!", 8);
    h->checksum = inet_checksum(out, 16);
    return 16;
}

static int simulate(void) {
    ui_printf("  last fragment: offset 8191 x 8 = 65528, length 100 -> ends at 65628\n");
    ui_printf("  a 65535-byte datagram buffer would be overrun by 93 bytes\n");
    ui_printf("  UzOS rebuilds at most %d bytes per datagram\n", IPV4_REASM_MAX);
    return 1;
}

static int launch(void) {
    netdev_t *eth = netdev_by_name("eth0");
    if (!eth || !eth->ip) {
        ui_printf("  needs eth0 with an address (dhclient)\n");
        return 0;
    }
    static uint8_t f[ETH_ZLEN + 1600];
    uint8_t data[64];

    static uint8_t chunk[104];
    uint32_t fails = ipv4_stats()->reasm_fails;
    netdev_inject(eth, f, (uint16_t)build_ip(f, eth, INET_ADDR(10, 0, 2, 98), eth->ip, IPPROTO_ICMP, 0x4444,
                                              IPV4_FLAG_MF, 0, 0, data, build_echo(data, 4, 0)));
    memset(chunk, 'X', sizeof(chunk));
    netdev_inject(eth, f, (uint16_t)build_ip(f, eth, INET_ADDR(10, 0, 2, 98), eth->ip, IPPROTO_ICMP, 0x4444,
                                              8191, 0, 0, chunk, sizeof(chunk)));
    int stored = ipv4_stats()->reasm_fails == fails;
    ui_printf("  fragment at offset 65528 (+%u bytes) -> %s\n", (unsigned)sizeof(chunk), stored ? "STORED" : "refused");
    ui_color(stored ? UI_BAD : UI_GOOD);
    ui_printf("  %s\n", stored ? "it was written past the end of the reassembly buffer" : "refused: the datagram was thrown away");
    ui_reset();
    return stored;
}

static int cleanup(void) {
    ipv4_fragment_flush();
    return 1;
}

static const attack_t attack = {
    "ping_of_death",
    "fragments whose offset + length reach past 65535 bytes (the 1996 ping of death)",
    INV_INTEGRITY, SEV_CRITICAL,
    simulate, launch, cleanup,
    "reassembly_bounds"
};

void atk_ping_of_death_register(void) { registry_add_attack(&attack); }

/*
 * atk_fragment_flood.c - ATTACK fragment_flood: fill the reassembly slots with datagrams that never end
 *
 * Breaks:     resource - the reassembly slots are shared fairly
 * Stopped by: frag_mem_limit (defence/frag_mem_limit)
 *
 * Each half-built datagram holds a slot (and its buffer) for 30 s. A
 * source that sends only first fragments takes every slot, and real
 * fragmented traffic from anyone else is lost.
 *
 * launch(): inject the first fragments of 4 datagrams from 10.0.2.96.
 *           findings = slots it holds beyond its share.
 * cleanup(): free the slots.
 *
 * Uses: netdev_inject (core/netdev), ipv4_reassembly_count / ipv4_fragment_flush (core/ipv4), ui_*.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/fragment_flood/defence/frag_mem_limit/def_frag_mem_limit.h" // FRAG_SOURCE_MAX
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

static int simulate(void) {
    ui_printf("  %d datagrams can be rebuilt at once; each waits up to %d s\n",
              IPV4_REASM_SLOTS, IPV4_REASM_TIMEOUT / 1000);
    ui_printf("  first fragments only, from one source, would hold all of them\n");
    return IPV4_REASM_SLOTS;
}

static int launch(void) {
    netdev_t *eth = netdev_by_name("eth0");
    if (!eth || !eth->ip) {
        ui_printf("  needs eth0 with an address (dhclient)\n");
        return 0;
    }
    static uint8_t f[ETH_ZLEN + 1600];

    uint32_t attacker = INET_ADDR(10, 0, 2, 96);
    static uint8_t chunk[16];
    for (int i = 0; i < IPV4_REASM_SLOTS; i++)
        netdev_inject(eth, f, (uint16_t)build_ip(f, eth, attacker, eth->ip, 99, (uint16_t)(0xF100 + i),
                                                  IPV4_FLAG_MF, 0, 0, chunk, sizeof(chunk)));
    int held = ipv4_reassembly_count(attacker);
    int extra = held > FRAG_SOURCE_MAX ? held - FRAG_SOURCE_MAX : 0;
    ui_printf("  one source holds %d of %d slots (its share: %d)\n", held, IPV4_REASM_SLOTS, FRAG_SOURCE_MAX);
    ui_color(extra ? UI_BAD : UI_GOOD);
    ui_printf("  %s\n", extra ? "every slot is taken: nobody else's fragments can be rebuilt" : "the other slots stay free for everyone else");
    ui_reset();
    return extra;
}

static int cleanup(void) {
    ipv4_fragment_flush();
    return 1;
}

static const attack_t attack = {
    "fragment_flood",
    "first fragments of many datagrams that never complete, from one source",
    INV_RESOURCE, SEV_MEDIUM,
    simulate, launch, cleanup,
    "frag_mem_limit"
};

void atk_fragment_flood_register(void) { registry_add_attack(&attack); }

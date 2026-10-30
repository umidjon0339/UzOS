/*
 * atk_icmp_redirect.c - ATTACK icmp_redirect: rewrite our routes with a forged redirect
 *
 * Breaks:     trust - only the administrator changes the routing table
 * Stopped by: no_accept_redirects (defence/no_accept_redirects)
 *
 * A router may tell a host "there is a better router for that address"
 * (ICMP type 5). A forged one moves our traffic to the attacker without
 * touching ARP: a quiet man-in-the-middle.
 *
 * launch(): inject a redirect "from" the gateway, for 8.8.8.8 via
 *           10.0.2.66. findings = 1 if our route changed.
 * cleanup(): delete the host route if it was added.
 *
 * Uses: netdev_inject (core/netdev), route_lookup / route_del (core/route), ui_*.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/icmp_redirect/defence/no_accept_redirects/def_no_accept_redirects.h" // def_redirect_ok
#include "modules/network_management/core/netdev/netdev.h"                      // netdev_by_name, netdev_inject, netdev_lock
#include "modules/network_management/core/ethernet/ethernet.h"                  // eth_header_t, ETH_*
#include "modules/network_management/core/ipv4/ipv4.h"                          // ipv4_header_t, ipv4_stats
#include "modules/network_management/core/icmp/icmp.h"                          // icmp_header_t, icmp_stats
#include "modules/network_management/core/inet/inet.h"                          // htons, inet_checksum, INET_ADDR
#include "lib/string/string.h"                                                  // memcpy, memset
#include "modules/network_management/core/route/route.h"                        // route_lookup, route_del
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
    ui_printf("  ICMP redirect (type 5): 'for 8.8.8.8 use 10.0.2.66'\n");
    ui_printf("  a host that obeys sends 8.8.8.8's traffic to the attacker\n");
    return 1;
}

static int launch(void) {
    netdev_t *eth = netdev_by_name("eth0");
    if (!eth || !eth->ip) {
        ui_printf("  needs eth0 with an address (dhclient)\n");
        return 0;
    }
    static uint8_t f[ETH_ZLEN + 1600];

    static uint8_t msg[ICMP_HLEN + IPV4_HLEN + 8];
    memset(msg, 0, sizeof(msg));
    icmp_header_t *h = (icmp_header_t *)msg;
    h->type = ICMP_REDIRECT;
    h->code = 1;                                // for this host
    uint32_t new_gw = htonl(INET_ADDR(10, 0, 2, 66));
    memcpy(&h->id, &new_gw, 4);                 // the new router sits where id and seq are
    ipv4_header_t *quoted = (ipv4_header_t *)(msg + ICMP_HLEN);   // "your packet to 8.8.8.8"
    quoted->version_ihl = 0x45;
    quoted->protocol = IPPROTO_ICMP;
    quoted->src = htonl(eth->ip);
    quoted->dst = htonl(INET_ADDR(8, 8, 8, 8));
    h->checksum = inet_checksum(msg, sizeof(msg));
    netdev_inject(eth, f, (uint16_t)build_ip(f, eth, INET_ADDR(10, 0, 2, 2), eth->ip, IPPROTO_ICMP, 7, 0, 0, 0, msg, sizeof(msg)));
    route_result_t r;
    int moved = route_lookup(INET_ADDR(8, 8, 8, 8), &r) && r.next_hop == INET_ADDR(10, 0, 2, 66);
    ui_color(moved ? UI_BAD : UI_GOOD);
    ui_printf("  %s\n", moved ? "8.8.8.8 now goes through the attacker" : "the redirect was ignored: routes unchanged");
    ui_reset();
    return moved;
}

static int cleanup(void) {
    uint32_t lock = netdev_lock();
    int r = route_del(INET_ADDR(8, 8, 8, 8), 0xFFFFFFFF) == 0;
    netdev_unlock(lock);
    return r;
}

static const attack_t attack = {
    "icmp_redirect",
    "a forged ICMP redirect: \"for 8.8.8.8, use 10.0.2.66 as the router\"",
    INV_TRUST, SEV_HIGH,
    simulate, launch, cleanup,
    "no_accept_redirects"
};

void atk_icmp_redirect_register(void) { registry_add_attack(&attack); }

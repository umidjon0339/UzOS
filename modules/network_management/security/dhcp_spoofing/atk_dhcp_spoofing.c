/*
 * atk_dhcp_spoofing.c - ATTACK dhcp_spoofing: a rogue DHCP server
 *
 * Breaks:     trust - our address, router and DNS come from the real DHCP server
 * Stopped by: dhcp_validate (defence/dhcp_validate)
 *
 * DHCP asks by broadcast, so anyone on the link hears it (xid included)
 * and can answer. The first OFFER wins: the attacker's router and DNS
 * server are now ours, a man-in-the-middle for everything.
 *
 * launch():  restart DHCP and, at the same moment, inject an OFFER from
 *            10.0.2.66 (router and DNS 10.0.2.66). findings = 1 if we
 *            took the rogue lease.
 * cleanup(): ask DHCP again.
 *
 * Uses: dhcp_start / dhcp_lease (core/dhcp), netdev_inject (core/netdev), ui_*.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/dhcp_spoofing/defence/dhcp_validate/def_dhcp_validate.h" // def_dhcp_offer_ok
#include "modules/network_management/core/netdev/netdev.h"                      // netdev_by_name, netdev_inject, netdev_lock
#include "modules/network_management/core/ethernet/ethernet.h"                  // eth_header_t, ETH_*
#include "modules/network_management/core/ipv4/ipv4.h"                          // ipv4_header_t, ipv4_stats
#include "modules/network_management/core/icmp/icmp.h"                          // icmp_header_t, icmp_stats
#include "modules/network_management/core/inet/inet.h"                          // htons, inet_checksum, INET_ADDR
#include "lib/string/string.h"                                                  // memcpy, memset
#include "modules/network_management/core/udp/udp.h"                            // udp_header_t, UDP_HLEN
#include "modules/network_management/core/dhcp/dhcp.h"                          // dhcp_start, dhcp_lease
#include "modules/process_management/syscalls/sleep/sys_sleep.h"                // sys_sleep
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

// A UDP header and its data, checksum filled in. Returns the length.
static int build_udp(uint8_t *out, uint32_t src, uint32_t dst, uint16_t sport, uint16_t dport,
                     const void *data, int len) {
    udp_header_t *h = (udp_header_t *)out;
    h->src_port = htons(sport);
    h->dst_port = htons(dport);
    h->length = htons((uint16_t)(UDP_HLEN + len));
    h->checksum = 0;
    memcpy(out + UDP_HLEN, data, len);
    h->checksum = inet_checksum_pseudo(src, dst, IPPROTO_UDP, out, UDP_HLEN + len);
    return UDP_HLEN + len;
}

#define ROGUE INET_ADDR(10, 0, 2, 66)

static void put_ip(uint8_t *at, uint32_t ip) {
    at[0] = (uint8_t)(ip >> 24); at[1] = (uint8_t)(ip >> 16); at[2] = (uint8_t)(ip >> 8); at[3] = (uint8_t)ip;
}

// A DHCPOFFER for our xid and MAC: the address 10.0.2.77, everything else the rogue.
static int build_offer(uint8_t *m, uint32_t xid, const uint8_t mac[6]) {
    memset(m, 0, 300);
    m[0] = 2;                               // BOOTREPLY
    m[1] = 1;
    m[2] = 6;
    put_ip(m + 4, xid);
    put_ip(m + 16, INET_ADDR(10, 0, 2, 77)); // yiaddr: "your address"
    memcpy(m + 28, mac, 6);                 // chaddr
    put_ip(m + 236, 0x63825363);            // the magic cookie
    uint8_t *o = m + 240;
    *o++ = 53; *o++ = 1; *o++ = DHCPOFFER;
    *o++ = 54; *o++ = 4; put_ip(o, ROGUE); o += 4;              // server id
    *o++ = 1;  *o++ = 4; put_ip(o, 0xFFFFFF00); o += 4;         // netmask
    *o++ = 3;  *o++ = 4; put_ip(o, ROGUE); o += 4;              // router
    *o++ = 6;  *o++ = 4; put_ip(o, ROGUE); o += 4;              // DNS
    *o++ = 51; *o++ = 4; put_ip(o, 3600); o += 4;               // lease
    *o = 255;
    return 300;
}

static int simulate(void) {
    ui_printf("  DHCP asks by broadcast: anyone may answer, and the first offer wins\n");
    ui_printf("  a rogue offer makes the attacker our router and our DNS server\n");
    return 1;
}

static int launch(void) {
    netdev_t *eth = netdev_by_name("eth0");
    if (!eth || dhcp_lease()->state != DHCP_BOUND) {
        ui_printf("  needs a DHCP lease first (dhclient)\n");
        return 0;
    }
    static uint8_t f[ETH_ZLEN + 400], data[400], msg[300];
    uint32_t lock = netdev_lock();
    dhcp_start(eth);                            // DISCOVER goes out ...
    int n = build_udp(data, ROGUE, 0xFFFFFFFF, 67, 68, msg, build_offer(msg, dhcp_lease()->xid, eth->mac));
    netdev_inject(eth, f, (uint16_t)build_ip(f, eth, ROGUE, 0xFFFFFFFF, IPPROTO_UDP, 1, 0, 0, 0, data, n));
    netdev_unlock(lock);                        // ... and the rogue answered before the real server
    for (int i = 0; i < 30 && dhcp_lease()->state != DHCP_BOUND; i++)
        sys_sleep(50);
    const dhcp_lease_t *l = dhcp_lease();
    char server[INET_ADDR_TEXT], router[INET_ADDR_TEXT];
    inet_format(l->server, server);
    inet_format(l->router, router);
    ui_printf("  lease from %s, router %s\n", server, router);
    int rogue = l->server == ROGUE;
    ui_color(rogue ? UI_BAD : UI_GOOD);
    ui_printf("  %s\n", rogue ? "the attacker is our router and DNS now" : "the rogue offer was refused; the real server's lease stands");
    ui_reset();
    return rogue;
}

static int cleanup(void) {
    netdev_t *eth = netdev_by_name("eth0");
    if (!eth || dhcp_lease()->server != ROGUE)
        return 0;
    uint32_t lock = netdev_lock();
    dhcp_start(eth);
    netdev_unlock(lock);
    return 1;
}

static const attack_t attack = {
    "dhcp_spoofing",
    "a rogue DHCP server answers first: its router and DNS server are the attacker",
    INV_TRUST, SEV_HIGH,
    simulate, launch, cleanup,
    "dhcp_validate"
};

void atk_dhcp_spoofing_register(void) { registry_add_attack(&attack); }

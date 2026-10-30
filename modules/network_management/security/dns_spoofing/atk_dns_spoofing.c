/*
 * atk_dns_spoofing.c - ATTACK dns_spoofing: win the race against the real DNS server
 *
 * Breaks:     trust - only the real DNS server's answer is believed
 * Stopped by: dns_txid_port_random (defence/dns_txid_port_random)
 *
 * A resolver takes the first answer whose id (16 bits) and port match
 * its question. If the port is predictable, a forger has 65536 ids to
 * try; if both are random, about 4 billion (Kaminsky, 2008: random
 * source ports became mandatory).
 *
 * launch(): ask 3 questions and watch their ids and source ports on the
 *           tap. findings = values that simply count up.
 *
 * Uses: dns_query (core/dns), netdev_set_tap (core/netdev), ui_*.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/dns_spoofing/defence/dns_txid_port_random/def_dns_txid_port_random.h" // the defence's ranges
#include "modules/network_management/core/netdev/netdev.h"                      // netdev_by_name, netdev_inject, netdev_lock
#include "modules/network_management/core/ethernet/ethernet.h"                  // eth_header_t, ETH_*
#include "modules/network_management/core/ipv4/ipv4.h"                          // ipv4_header_t, ipv4_stats
#include "modules/network_management/core/icmp/icmp.h"                          // icmp_header_t, icmp_stats
#include "modules/network_management/core/inet/inet.h"                          // htons, inet_checksum, INET_ADDR
#include "lib/string/string.h"                                                  // memcpy, memset
#include "modules/network_management/core/udp/udp.h"                            // udp_header_t, UDP_HLEN
#include "modules/network_management/core/dns/dns.h"                            // dns_query
#include "modules/ui/core/style/ui.h"                                           // ui_printf, ui_color

static volatile uint16_t ports[4], ids[4];
static volatile int seen;

// In the interrupt: our queries to port 53.
static void tap(netdev_t *dev, const uint8_t *frame, uint16_t len, int outgoing) {
    (void)dev;
    if (!outgoing || seen >= 4 || len < ETH_HLEN + IPV4_HLEN + UDP_HLEN + 2)
        return;
    const ipv4_header_t *ip = (const ipv4_header_t *)(frame + ETH_HLEN);
    const udp_header_t *u = (const udp_header_t *)(frame + ETH_HLEN + IPV4_HLEN);
    if (ip->protocol != IPPROTO_UDP || ntohs(u->dst_port) != 53)
        return;
    const uint8_t *dns = frame + ETH_HLEN + IPV4_HLEN + UDP_HLEN;
    ports[seen] = ntohs(u->src_port);
    ids[seen] = (uint16_t)((dns[0] << 8) | dns[1]);
    seen++;
}

static int simulate(void) {
    ui_printf("  a forged answer must match the query's id (16 bits) and source port\n");
    ui_printf("  counting ports or ids make the guess easy (Kaminsky, 2008)\n");
    return 1;
}

static int launch(void) {
    static dns_result_t r;
    seen = 0;
    uint32_t lock = netdev_lock();
    netdev_set_tap(tap);
    netdev_unlock(lock);
    static const char *names[3] = { "example.com", "example.org", "example.net" };
    for (int i = 0; i < 3; i++)
        dns_query(names[i], DNS_TYPE_A, 0, &r);
    lock = netdev_lock();
    netdev_set_tap(0);
    netdev_unlock(lock);
    if (seen < 2) {
        ui_printf("  no DNS queries seen (is there a DNS server? dhclient)\n");
        return 0;
    }
    int counting = 0;
    for (int i = 0; i < seen; i++) {
        ui_printf("  query %d: source port %5u, id %5u\n", i + 1, ports[i], ids[i]);
        if (i && ((uint16_t)(ports[i] - ports[i - 1]) <= 2 || (uint16_t)(ids[i] - ids[i - 1]) <= 2))
            counting++;
    }
    ui_color(counting ? UI_BAD : UI_GOOD);
    ui_printf("  %s\n", counting ? "the next query's numbers can be guessed" : "port and id are random: a forger must guess 32 bits");
    ui_reset();
    return counting;
}

static int cleanup(void) {
    return 0;
}

static const attack_t attack = {
    "dns_spoofing",
    "guess a DNS query's id and source port, and answer first with a forged address",
    INV_TRUST, SEV_HIGH,
    simulate, launch, cleanup,
    "dns_txid_port_random"
};

void atk_dns_spoofing_register(void) { registry_add_attack(&attack); }

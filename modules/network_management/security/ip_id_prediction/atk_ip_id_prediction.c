/*
 * atk_ip_id_prediction.c - ATTACK ip_id_prediction: read our traffic off the IP id
 *
 * Breaks:     disclosure - our packets do not reveal how many packets we send
 * Stopped by: random_ip_id (defence/random_ip_id)
 *
 * If every packet's IP id is the previous one + 1, anyone who sees two
 * of our packets knows how many we sent in between. nmap's "idle scan"
 * uses exactly that to scan a third host through us without ever
 * showing its own address.
 *
 * launch(): send 5 pings and read their ids off the tap.
 *           findings = ids that were exactly the previous + 1.
 *
 * Uses: netdev_set_tap (core/netdev), icmp_send_echo (core/icmp), sys_sleep, ui_*.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/ip_id_prediction/defence/random_ip_id/def_random_ip_id.h" // def_next_ip_id
#include "modules/network_management/core/netdev/netdev.h"                      // netdev_by_name, netdev_inject, netdev_lock
#include "modules/network_management/core/ethernet/ethernet.h"                  // eth_header_t, ETH_*
#include "modules/network_management/core/ipv4/ipv4.h"                          // ipv4_header_t, ipv4_stats
#include "modules/network_management/core/icmp/icmp.h"                          // icmp_header_t, icmp_stats
#include "modules/network_management/core/inet/inet.h"                          // htons, inet_checksum, INET_ADDR
#include "lib/string/string.h"                                                  // memcpy, memset
#include "modules/process_management/syscalls/sleep/sys_sleep.h"                // sys_sleep
#include "modules/ui/core/style/ui.h"                                           // ui_printf, ui_color

static volatile uint16_t ids[8];
static volatile int seen;

// In the interrupt: the id of each echo request we send.
static void tap(netdev_t *dev, const uint8_t *frame, uint16_t len, int outgoing) {
    (void)dev;
    if (!outgoing || seen >= 8 || len < ETH_HLEN + IPV4_HLEN + 1 || frame[12] != 0x08 || frame[13] != 0)
        return;
    const ipv4_header_t *ip = (const ipv4_header_t *)(frame + ETH_HLEN);
    if (ip->protocol == IPPROTO_ICMP && frame[ETH_HLEN + IPV4_HLEN] == ICMP_ECHO_REQUEST)
        ids[seen++] = ntohs(ip->id);
}

static int simulate(void) {
    ui_printf("  a counter id: packet n+1 has id(n)+1, so the gap between two of\n");
    ui_printf("  our packets = how many we sent meanwhile (nmap -sI, idle scan)\n");
    return 1;
}

static int launch(void) {
    netdev_t *eth = netdev_by_name("eth0");
    if (!eth || !eth->ip) {
        ui_printf("  needs eth0 with an address\n");
        return 0;
    }
    seen = 0;
    uint32_t lock = netdev_lock();
    netdev_set_tap(tap);
    netdev_unlock(lock);
    for (int i = 0; i < 5; i++) {
        lock = netdev_lock();
        icmp_send_echo(INET_ADDR(10, 0, 2, 2), 0x1D1D, (uint16_t)i, 0, "id", 2);
        netdev_unlock(lock);
        sys_sleep(20);
    }
    lock = netdev_lock();
    netdev_set_tap(0);
    netdev_unlock(lock);
    int counted = 0;
    ui_printf("  ids:");
    for (int i = 0; i < seen; i++) {
        ui_printf(" %u", ids[i]);
        if (i && (uint16_t)(ids[i] - ids[i - 1]) == 1)
            counted++;
    }
    ui_printf("\n");
    ui_color(counted ? UI_BAD : UI_GOOD);
    ui_printf("  %s\n", counted ? "a counter: our traffic can be read off the ids" : "unpredictable: the ids tell nothing");
    ui_reset();
    return counted;
}

static int cleanup(void) {
    return 0;
}

static const attack_t attack = {
    "ip_id_prediction",
    "watch the IP id of our packets: a counter tells others our traffic (idle scan)",
    INV_DISCLOSURE, SEV_LOW,
    simulate, launch, cleanup,
    "random_ip_id"
};

void atk_ip_id_prediction_register(void) { registry_add_attack(&attack); }

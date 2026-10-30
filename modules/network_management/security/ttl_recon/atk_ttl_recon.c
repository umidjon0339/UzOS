/*
 * atk_ttl_recon.c - ATTACK ttl_recon (ILLUSTRATION): fingerprint the OS from the TTL
 *
 * Breaks:     disclosure - our packets say as little as possible about what we run
 * Stopped by: ttl_normalize (defence/ttl_normalize) - ILLUSTRATION
 *
 * Every OS starts its packets with its own TTL; scanners (nmap -O, p0f)
 * guess the OS from it. UzOS sends the most common value, 64, so it
 * looks like "some Unix" and nothing more. An ILLUSTRATION: the leak is
 * small and cannot be closed, only made unremarkable.
 *
 * launch(): ping ourselves and read the TTL. findings = 1 if it is
 *           unusual enough to single us out.
 *
 * Uses: icmp_send_echo / icmp_set_observer (core/icmp), sys_sleep, ui_*.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/ttl_recon/defence/ttl_normalize/def_ttl_normalize.h" // def_default_ttl
#include "modules/network_management/core/netdev/netdev.h"                      // netdev_by_name, netdev_inject, netdev_lock
#include "modules/network_management/core/ethernet/ethernet.h"                  // eth_header_t, ETH_*
#include "modules/network_management/core/ipv4/ipv4.h"                          // ipv4_header_t, ipv4_stats
#include "modules/network_management/core/icmp/icmp.h"                          // icmp_header_t, icmp_stats
#include "modules/network_management/core/inet/inet.h"                          // htons, inet_checksum, INET_ADDR
#include "lib/string/string.h"                                                  // memcpy, memset
#include "modules/process_management/syscalls/sleep/sys_sleep.h"                // sys_sleep
#include "modules/ui/core/style/ui.h"                                           // ui_printf, ui_color

static volatile int ttl = -1;

static void heard(const icmp_event_t *e) {
    if (e->type == ICMP_ECHO_REPLY && e->id == 0x7171)
        ttl = e->ttl;
}

static int simulate(void) {
    ui_printf("  initial TTL 64: Linux / Unix, 128: Windows, 255: routers\n");
    ui_printf("  UzOS uses %d: it hides among the most common systems\n", def_default_ttl());
    return 1;
}

static int launch(void) {
    ttl = -1;
    icmp_set_observer(heard);
    uint32_t lock = netdev_lock();
    icmp_send_echo(INET_ADDR(127, 0, 0, 1), 0x7171, 1, 0, "ttl", 3);
    netdev_unlock(lock);
    sys_sleep(50);
    icmp_set_observer(0);
    int odd = ttl >= 0 && ttl != 64 && ttl != 128;
    ui_printf("  our TTL: %d -> looks like %s\n", ttl, ttl == 64 ? "Linux / Unix" : ttl == 128 ? "Windows" : "something rare");
    return odd;
}

static int cleanup(void) {
    return 0;
}

static const attack_t attack = {
    "ttl_recon",
    "guess the OS from the TTL of its packets (64 Linux, 128 Windows, 255 network gear)",
    INV_DISCLOSURE, SEV_INFO,
    simulate, launch, cleanup,
    "ttl_normalize"
};

void atk_ttl_recon_register(void) { registry_add_attack(&attack); }

/*
 * atk_plaintext_sniffing.c - ATTACK plaintext_sniffing (ILLUSTRATION): read a password off the wire
 *
 * Breaks:     disclosure - a password travels unreadable to anyone on the path
 * Stopped by: encryption (defence/encryption) - ILLUSTRATION
 *
 * telnet, FTP and plain HTTP send everything as it is typed. Anyone who
 * sees the packets (a sniffer on the link, a router on the way) reads
 * the password. An ILLUSTRATION: UzOS has no TLS or SSH, so nothing
 * stops this here; the point is to see why encryption exists.
 *
 * launch(): over lo, send "USER root / PASS uzos" on a TCP connection
 *           and read it back off the tap. findings = 1 (it is readable).
 *
 * Uses: socket_* (core/socket), netdev_set_tap (core/netdev), ui_*.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/plaintext_sniffing/defence/encryption/def_encryption.h" // def_plaintext_port
#include "modules/network_management/core/netdev/netdev.h"                      // netdev_by_name, netdev_inject, netdev_lock
#include "modules/network_management/core/ethernet/ethernet.h"                  // eth_header_t, ETH_*
#include "modules/network_management/core/ipv4/ipv4.h"                          // ipv4_header_t, ipv4_stats
#include "modules/network_management/core/icmp/icmp.h"                          // icmp_header_t, icmp_stats
#include "modules/network_management/core/inet/inet.h"                          // htons, inet_checksum, INET_ADDR
#include "lib/string/string.h"                                                  // memcpy, memset
#include "modules/network_management/core/tcp/tcp.h"                            // tcp_header_t
#include "modules/network_management/core/socket/socket.h"                      // socket_*
#include "modules/ui/core/style/ui.h"                                           // ui_printf, ui_color

#define PORT 7784
static char caught[64];
static volatile int caught_len;

// In the interrupt: keep the data of the first segment that carries "PASS".
static void tap(netdev_t *dev, const uint8_t *frame, uint16_t len, int outgoing) {
    (void)dev;
    (void)outgoing;
    if (caught_len || len < ETH_HLEN + IPV4_HLEN + TCP_HLEN)
        return;
    const ipv4_header_t *ip = (const ipv4_header_t *)(frame + ETH_HLEN);
    if (ip->protocol != IPPROTO_TCP)
        return;
    const tcp_header_t *t = (const tcp_header_t *)(frame + ETH_HLEN + IPV4_HLEN);
    const uint8_t *data = (const uint8_t *)t + (t->offset >> 4) * 4;
    int n = (int)(frame + len - data);
    for (int i = 0; i + 4 <= n; i++) {
        if (data[i] == 'P' && data[i + 1] == 'A' && data[i + 2] == 'S' && data[i + 3] == 'S') {
            int k = 0;
            while (i + k < n && k < 63 && data[i + k] != '\r')
                caught[k] = (char)data[i + k], k++;
            caught[k] = '\0';
            caught_len = k;
            return;
        }
    }
}

static int simulate(void) {
    ui_printf("  telnet / FTP / HTTP: the password is in the packet as typed\n");
    ui_printf("  tcpdump -A shows it; TLS or SSH turn it into noise\n");
    return 1;
}

static int launch(void) {
    socket_t *l, *c, *s;
    sockaddr_t a;
    uint32_t len = sizeof(a);
    if (socket_create(AF_INET, SOCK_STREAM, 0, &l) < 0)
        return 0;
    int one = 1;
    socket_setopt(l, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    socket_inet_addr(&a, &len, 0, PORT);
    if (socket_bind(l, &a, len) < 0 || socket_listen(l, 1) < 0 || socket_create(AF_INET, SOCK_STREAM, 0, &c) < 0) {
        socket_close(l);
        return 0;
    }
    caught_len = 0;
    uint32_t lock = netdev_lock();
    netdev_set_tap(tap);
    netdev_unlock(lock);
    len = sizeof(a);
    socket_inet_addr(&a, &len, INET_ADDR(127, 0, 0, 1), PORT);
    if (socket_connect(c, &a, len) == 0 && socket_accept(l, &s) == 0) {
        socket_sendto(c, "USER root\r\nPASS uzos\r\n", 22, 0, 0, 0);
        socket_close(s);
    }
    lock = netdev_lock();
    netdev_set_tap(0);
    netdev_unlock(lock);
    socket_close(c);
    socket_close(l);
    if (caught_len) {
        ui_color(UI_BAD);
        ui_printf("  sniffed off the wire: \"%s\"\n", caught);
        ui_reset();
    }
    ui_printf("  UzOS has no TLS / SSH: plaintext protocols are readable (see the README)\n");
    return caught_len ? 1 : 0;
}

static int cleanup(void) {
    return 0;
}

static const attack_t attack = {
    "plaintext_sniffing",
    "read a telnet-style login straight off the wire",
    INV_DISCLOSURE, SEV_HIGH,
    simulate, launch, cleanup,
    "encryption"
};

void atk_plaintext_sniffing_register(void) { registry_add_attack(&attack); }

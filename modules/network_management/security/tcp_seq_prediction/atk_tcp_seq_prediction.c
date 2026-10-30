/*
 * atk_tcp_seq_prediction.c - ATTACK tcp_seq_prediction: guess the next connection's sequence numbers
 *
 * Breaks:     trust - nobody can guess the sequence numbers of a connection they cannot see
 * Stopped by: random_isn (defence/random_isn)
 *
 * Old stacks started each connection at a counter (+64000 per
 * connection, +128000 per second). Seeing one ISN told you the next:
 * Mitnick (1994) forged a whole connection from a trusted host that way.
 *
 * launch(): open 4 connections over lo and compare the server's ISNs.
 *           findings = consecutive ISNs closer than 2^20.
 *
 * Uses: socket_* (core/socket), tcp_conn_t (core/tcp), ui_*.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/tcp_seq_prediction/defence/random_isn/def_random_isn.h" // def_isn
#include "modules/network_management/core/netdev/netdev.h"                      // netdev_by_name, netdev_inject, netdev_lock
#include "modules/network_management/core/ethernet/ethernet.h"                  // eth_header_t, ETH_*
#include "modules/network_management/core/ipv4/ipv4.h"                          // ipv4_header_t, ipv4_stats
#include "modules/network_management/core/icmp/icmp.h"                          // icmp_header_t, icmp_stats
#include "modules/network_management/core/inet/inet.h"                          // htons, inet_checksum, INET_ADDR
#include "lib/string/string.h"                                                  // memcpy, memset
#include "modules/network_management/core/tcp/tcp.h"                            // tcp_header_t, tcp_conn_t, TCP_*
#include "modules/network_management/core/socket/socket.h"                      // socket_*
#include "modules/ui/core/style/ui.h"                                           // ui_printf, ui_color

// A TCP connection over lo: listener, client, and the accepted server
// end. 0, or -1 if something failed (all closed again).
static int make_pair(uint16_t port, socket_t **l, socket_t **client, socket_t **server) {
    sockaddr_t a;
    uint32_t len = sizeof(a);
    *l = *client = *server = 0;
    if (socket_create(AF_INET, SOCK_STREAM, 0, l) < 0)
        return -1;
    int one = 1;                            // the last run's ends may still hold the port
    socket_setopt(*l, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    socket_inet_addr(&a, &len, 0, port);
    if (socket_bind(*l, &a, len) < 0 || socket_listen(*l, 4) < 0 ||
        socket_create(AF_INET, SOCK_STREAM, 0, client) < 0) {
        socket_close(*l);
        return -1;
    }
    len = sizeof(a);
    socket_inet_addr(&a, &len, INET_ADDR(127, 0, 0, 1), port);
    if (socket_connect(*client, &a, len) < 0 || socket_accept(*l, server) < 0) {
        socket_close(*client);
        socket_close(*l);
        return -1;
    }
    return 0;
}

static void close_pair(socket_t *l, socket_t *client, socket_t *server) {
    if (server) socket_close(server);
    if (client) socket_close(client);
    if (l)      socket_close(l);
}
#define PORT 7778

static int simulate(void) {
    ui_printf("  a counter ISN: the next connection starts where the last ended + a step\n");
    ui_printf("  one look tells an attacker what to forge (Mitnick, 1994)\n");
    return 1;
}

static int launch(void) {
    uint32_t isn[4];
    int got = 0;
    for (int i = 0; i < 4; i++) {
        socket_t *l, *c, *s;
        if (make_pair(PORT, &l, &c, &s) < 0)
            break;
        isn[got++] = ((tcp_conn_t *)s->proto)->iss;      // the server's first sequence number
        close_pair(l, c, s);
    }
    int close = 0;
    ui_printf("  server ISNs:");
    for (int i = 0; i < got; i++) {
        ui_printf(" %u", isn[i]);
        if (i) {
            uint32_t step = isn[i] - isn[i - 1];
            close += step < (1u << 20) || step > 0u - (1u << 20);
        }
    }
    ui_printf("\n");
    ui_color(close ? UI_BAD : UI_GOOD);
    ui_printf("  %s\n", close ? "the next ISN can be guessed" : "no pattern: each ISN is unguessable");
    ui_reset();
    return close;
}

static int cleanup(void) {
    return 0;
}

static const attack_t attack = {
    "tcp_seq_prediction",
    "open connections and look for a pattern in the server's initial sequence numbers",
    INV_TRUST, SEV_HIGH,
    simulate, launch, cleanup,
    "random_isn"
};

void atk_tcp_seq_prediction_register(void) { registry_add_attack(&attack); }

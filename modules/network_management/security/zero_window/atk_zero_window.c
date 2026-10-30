/*
 * atk_zero_window.c - ATTACK zero_window: hold a connection open with a window of zero
 *
 * Breaks:     resource - a peer cannot hold our send buffer forever
 * Stopped by: persist_timeout (defence/persist_timeout)
 *
 * A receiver that never reads advertises window 0. The sender keeps its
 * unsent data and probes politely, forever, by RFC. Many such peers tie
 * up all of a server's buffers (sockstress, 2008).
 *
 * launch(): make a connection over lo; the receiving end never reads,
 *           the sending end writes 12 KiB. Wait 7 s (the probes back off:
 *           0.2 + 0.4 + 0.8 + 1.6 + 3.2 s).
 *           findings = 1 if the sender still holds the connection.
 *
 * Uses: socket_* (core/socket), tcp_conn_t (core/tcp), sys_sleep, ui_*.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/zero_window/defence/persist_timeout/def_persist_timeout.h" // PERSIST_MAX_PROBES
#include "modules/network_management/core/netdev/netdev.h"                      // netdev_by_name, netdev_inject, netdev_lock
#include "modules/network_management/core/ethernet/ethernet.h"                  // eth_header_t, ETH_*
#include "modules/network_management/core/ipv4/ipv4.h"                          // ipv4_header_t, ipv4_stats
#include "modules/network_management/core/icmp/icmp.h"                          // icmp_header_t, icmp_stats
#include "modules/network_management/core/inet/inet.h"                          // htons, inet_checksum, INET_ADDR
#include "lib/string/string.h"                                                  // memcpy, memset
#include "modules/network_management/core/tcp/tcp.h"                            // tcp_header_t, tcp_conn_t, TCP_*
#include "modules/network_management/core/socket/socket.h"                      // socket_*
#include "modules/process_management/syscalls/sleep/sys_sleep.h"                // sys_sleep
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
#define PORT 7781

static int simulate(void) {
    ui_printf("  the receiver never reads: its window falls to 0 and stays there\n");
    ui_printf("  the sender probes; after %d probes it gives up\n", PERSIST_MAX_PROBES);
    return 1;
}

static int launch(void) {
    socket_t *l, *c, *s;
    if (make_pair(PORT, &l, &c, &s) < 0)
        return 0;
    tcp_conn_t *sender = c->proto;
    static uint8_t junk[2048];
    c->nonblock = 1;
    for (int i = 0; i < 6; i++)
        socket_sendto(c, junk, sizeof(junk), 0, 0, 0);
    ui_printf("  the receiver has %u bytes and reads none; window %u\n", ((tcp_conn_t *)s->proto)->rcv_len, sender->snd_wnd);
    sys_sleep(7000);
    int held = sender->used && sender->state == TCP_ESTABLISHED;
    ui_printf("  after 7 s the sender's connection is %s\n", sender->used ? tcp_state_name(sender->state) : "gone");
    close_pair(l, c, s);
    ui_color(held ? UI_BAD : UI_GOOD);
    ui_printf("  %s\n", held ? "the stalled peer holds our buffer forever" : "the sender gave up on the silent window");
    ui_reset();
    return held;
}

static int cleanup(void) {
    return 0;
}

static const attack_t attack = {
    "zero_window",
    "accept data, never read it, keep the window at 0 (sockstress)",
    INV_RESOURCE, SEV_MEDIUM,
    simulate, launch, cleanup,
    "persist_timeout"
};

void atk_zero_window_register(void) { registry_add_attack(&attack); }

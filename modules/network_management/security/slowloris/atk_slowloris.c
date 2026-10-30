/*
 * atk_slowloris.c - ATTACK slowloris: Slowloris: a request that never finishes
 *
 * Breaks:     liveness - a slow client cannot hold a server forever
 * Stopped by: idle_timeout (defence/idle_timeout)
 *
 * httpd serves one client at a time. A client that sends "GET / HT",
 * then one more byte every few seconds, never finishes its request: a
 * per-read timeout never fires, and nobody else is served (2009).
 *
 * launch(): needs httpd (httpd start). Connect to port 80, send part
 *           of a request, then a byte every 2 s. findings = 1 if the
 *           server still holds the connection after 8 s.
 *
 * Uses: socket_* (core/socket), sys_sleep, ui_*.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/slowloris/defence/idle_timeout/def_idle_timeout.h" // REQUEST_DEADLINE_MS
#include "modules/network_management/core/socket/socket.h"                      // socket_*
#include "modules/network_management/core/tcp/tcp.h"                            // tcp_conn_t, TCP_ESTABLISHED
#include "modules/network_management/core/inet/inet.h"                          // INET_ADDR, NET_E*
#include "modules/process_management/syscalls/sleep/sys_sleep.h"                // sys_sleep
#include "modules/ui/core/style/ui.h"                                           // ui_printf, ui_color

static int simulate(void) {
    ui_printf("  'GET / HT' ... 2 s ... 'T' ... 2 s ... 'P' ...: never finished\n");
    ui_printf("  each byte resets a per-read timeout; only a TOTAL deadline helps\n");
    ui_printf("  httpd allows %d s for the whole request\n", REQUEST_DEADLINE_MS / 1000);
    return 1;
}

static int launch(void) {
    socket_t *s;
    if (socket_create(AF_INET, SOCK_STREAM, 0, &s) < 0)
        return 0;
    sockaddr_t a;
    uint32_t len = sizeof(a);
    socket_inet_addr(&a, &len, INET_ADDR(127, 0, 0, 1), 80);
    if (socket_connect(s, &a, len) < 0) {
        ui_printf("  no web server on port 80: run 'httpd start' first\n");
        socket_close(s);
        return 0;
    }
    socket_sendto(s, "GET / HT", 8, 0, 0, 0);
    tcp_conn_t *conn = s->proto;
    int alive = 1;
    for (int t = 0; t < 4 && alive; t++) {
        sys_sleep(2000);
        socket_sendto(s, "T", 1, MSG_DONTWAIT, 0, 0);
        alive = conn->state == TCP_ESTABLISHED;     // a FIN or RST from the server ends it
        ui_printf("  after %d s: %s\n", (t + 1) * 2, alive ? "still held" : "the server dropped us");
    }
    socket_close(s);
    ui_color(alive ? UI_BAD : UI_GOOD);
    ui_printf("  %s\n", alive ? "the server is still stuck on us: nobody else is served" : "the server cut the slow request off");
    ui_reset();
    return alive;
}

static int cleanup(void) {
    return 0;
}

static const attack_t attack = {
    "slowloris",
    "send an HTTP request one byte now and then, so it never ends",
    INV_LIVENESS, SEV_MEDIUM,
    simulate, launch, cleanup,
    "idle_timeout"
};

void atk_slowloris_register(void) { registry_add_attack(&attack); }

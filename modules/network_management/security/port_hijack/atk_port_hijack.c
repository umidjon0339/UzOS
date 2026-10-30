/*
 * atk_port_hijack.c - ATTACK port_hijack: share someone else's port with SO_REUSEPORT
 *
 * Breaks:     isolation - another user cannot share (and steal from) my port
 * Stopped by: reuseport_owner (defence/reuseport_owner)
 *
 * SO_REUSEPORT lets several sockets bind the same port (a server's
 * worker processes). If ANY user could join the group, a stranger's
 * socket would receive some of the server's datagrams.
 *
 * launch(): uid 0 binds UDP 5353 with SO_REUSEPORT; a socket of uid 1
 *           tries to join. findings = 1 if it got in.
 *
 * Uses: socket_* (core/socket), ui_*.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/port_hijack/defence/reuseport_owner/def_reuseport_owner.h" // def_reuseport_ok
#include "modules/network_management/core/socket/socket.h"                      // socket_*
#include "modules/network_management/core/inet/inet.h"                          // INET_ADDR, NET_E*
#include "modules/ui/core/style/ui.h"                                           // ui_printf, ui_color

#define ATTACKER_UID 1
#define PORT 5353

static int simulate(void) {
    ui_printf("  SO_REUSEPORT: several sockets on one port, the traffic shared out\n");
    ui_printf("  a stranger in the group would get part of the server's datagrams\n");
    return 1;
}

static int launch(void) {
    socket_t *victim, *thief;
    if (socket_create(AF_INET, SOCK_DGRAM, 0, &victim) < 0)
        return 0;
    if (socket_create(AF_INET, SOCK_DGRAM, 0, &thief) < 0) {
        socket_close(victim);
        return 0;
    }
    int one = 1;
    socket_setopt(victim, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one));
    socket_setopt(thief, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one));
    thief->uid = ATTACKER_UID;
    sockaddr_t a;
    uint32_t len = sizeof(a);
    socket_inet_addr(&a, &len, 0, PORT);
    int v = socket_bind(victim, &a, len);
    int t = socket_bind(thief, &a, len);
    ui_printf("  uid 0 binds %d with SO_REUSEPORT -> %s\n", PORT, v == 0 ? "ok" : "busy");
    ui_printf("  uid %d joins the same port         -> %s\n", ATTACKER_UID, t == 0 ? "JOINED" : "refused");
    socket_close(thief);
    socket_close(victim);
    ui_color(t == 0 && v == 0 ? UI_BAD : UI_GOOD);
    ui_printf("  %s\n", t == 0 && v == 0 ? "the stranger shares the server's port" : "only the port's owner may add sockets to it");
    ui_reset();
    return t == 0 && v == 0;
}

static int cleanup(void) {
    return 0;
}

static const attack_t attack = {
    "port_hijack",
    "join another user's SO_REUSEPORT port and receive a share of its traffic",
    INV_ISOLATION, SEV_MEDIUM,
    simulate, launch, cleanup,
    "reuseport_owner"
};

void atk_port_hijack_register(void) { registry_add_attack(&attack); }

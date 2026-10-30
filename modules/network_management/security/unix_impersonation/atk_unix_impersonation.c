/*
 * atk_unix_impersonation.c - ATTACK unix_impersonation: claim to be root over a UNIX socket
 *
 * Breaks:     identity - a local server knows who its client really is
 * Stopped by: peercred_check (defence/peercred_check)
 *
 * A local service (a package manager, a password daemon) must know who
 * asks. A client can SAY anything; SO_PEERCRED tells what the KERNEL saw
 * when the client connected: its pid and uid, which it cannot change.
 *
 * launch(): a server on "/tmp/seclab.sock"; a client socket owned by
 *           uid 1 connects and sends "uid=0". The server asks SO_PEERCRED.
 *           findings = 1 if the kernel reported uid 0.
 *
 * Uses: socket_* (core/socket), AF_UNIX (core/unix), ui_*.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/unix_impersonation/defence/peercred_check/def_peercred_check.h" // def_peer_cred
#include "modules/network_management/core/socket/socket.h"                      // socket_*
#include "modules/network_management/core/inet/inet.h"                          // INET_ADDR, NET_E*
#include "modules/network_management/core/unix/unix.h"                          // sockaddr_un_t
#include "lib/string/string.h"                                                  // strcpy
#include "modules/ui/core/style/ui.h"                                           // ui_printf, ui_color

#define ATTACKER_UID 1

static int simulate(void) {
    ui_printf("  what a client says about itself proves nothing\n");
    ui_printf("  SO_PEERCRED: the kernel's own record of who connected\n");
    return 1;
}

static int launch(void) {
    socket_t *server, *client, *conn = 0;
    if (socket_create(AF_UNIX, SOCK_STREAM, 0, &server) < 0)
        return 0;
    if (socket_create(AF_UNIX, SOCK_STREAM, 0, &client) < 0) {
        socket_close(server);
        return 0;
    }
    client->uid = ATTACKER_UID;                 // a socket user 1 made
    sockaddr_un_t addr = { AF_UNIX, "" };
    strcpy(addr.path, "/tmp/seclab.sock");
    int fooled = 0;
    if (socket_bind(server, (sockaddr_t *)&addr, sizeof(addr)) == 0 && socket_listen(server, 1) == 0 &&
        socket_connect(client, (sockaddr_t *)&addr, sizeof(addr)) == 0 && socket_accept(server, &conn) == 0) {
        socket_sendto(client, "uid=0", 5, 0, 0, 0);
        char said[8] = "";
        socket_recvfrom(conn, said, 5, 0, 0, 0);
        socket_cred_t cred;
        uint32_t len = sizeof(cred);
        socket_getopt(conn, SOL_SOCKET, SO_PEERCRED, &cred, &len);
        ui_printf("  the client says \"%s\"; SO_PEERCRED says uid %u\n", said, cred.uid);
        fooled = cred.uid == 0;
    }
    if (conn)
        socket_close(conn);
    socket_close(client);
    socket_close(server);
    ui_color(fooled ? UI_BAD : UI_GOOD);
    ui_printf("  %s\n", fooled ? "the kernel vouched for a false uid" : "the kernel's record shows the real uid: the server refuses");
    ui_reset();
    return fooled;
}

static int cleanup(void) {
    return 0;
}

static const attack_t attack = {
    "unix_impersonation",
    "a local client claims \"I am root\"; only the kernel's answer counts",
    INV_IDENTITY, SEV_HIGH,
    simulate, launch, cleanup,
    "peercred_check"
};

void atk_unix_impersonation_register(void) { registry_add_attack(&attack); }

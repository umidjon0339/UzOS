/*
 * atk_raw_socket_abuse.c - ATTACK raw_socket_abuse: raw sockets for a normal user
 *
 * Breaks:     privilege - a normal user cannot forge whole IP packets
 * Stopped by: raw_cap (defence/raw_cap)
 *
 * A raw socket builds the protocol header itself: any source port, any
 * flags, any ICMP message (a forged RST, a redirect, a SYN flood), and it
 * receives copies of every packet of its protocol.
 *
 * launch(): ask socket(SOCK_RAW) for uid 1 (refused), then for root.
 *           findings = 1 if uid 1 was allowed.
 *
 * Uses: def_raw_socket_ok (the check socket() makes), socket_create (core/socket), ui_*.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/raw_socket_abuse/defence/raw_cap/def_raw_cap.h" // def_raw_socket_ok
#include "modules/network_management/core/socket/socket.h"                      // socket_*
#include "modules/network_management/core/inet/inet.h"                          // INET_ADDR, NET_E*
#include "modules/network_management/core/ipv4/ipv4.h"                          // IPPROTO_ICMP
#include "modules/ui/core/style/ui.h"                                           // ui_printf, ui_color

#define ATTACKER_UID 1

static int simulate(void) {
    ui_printf("  SOCK_RAW: you write the TCP/ICMP header yourself, any field you like\n");
    ui_printf("  and you get a copy of every packet of that protocol\n");
    return 1;
}

static int launch(void) {
    int user = def_raw_socket_ok(ATTACKER_UID);     // what socket() asks for uid 1
    ui_printf("  uid %d: socket(AF_INET, SOCK_RAW, IPPROTO_ICMP) -> %s\n", ATTACKER_UID, user ? "ALLOWED" : "EPERM");
    socket_t *s;
    int root = socket_create(AF_INET, SOCK_RAW, IPPROTO_ICMP, &s) == 0;
    ui_printf("  root (this shell):                  -> %s\n", root ? "allowed (CAP_NET_RAW)" : "refused");
    if (root)
        socket_close(s);
    ui_color(user ? UI_BAD : UI_GOOD);
    ui_printf("  %s\n", user ? "a user can forge packets" : "raw packets stay root's");
    ui_reset();
    return user;
}

static int cleanup(void) {
    return 0;
}

static const attack_t attack = {
    "raw_socket_abuse",
    "as a normal user, open a raw socket to forge and sniff IP packets",
    INV_PRIVILEGE, SEV_HIGH,
    simulate, launch, cleanup,
    "raw_cap"
};

void atk_raw_socket_abuse_register(void) { registry_add_attack(&attack); }

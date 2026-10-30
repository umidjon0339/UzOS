/*
 * atk_privileged_port_bind.c - ATTACK privileged_port_bind: a user takes a privileged port
 *
 * Breaks:     privilege - only the administrator runs services on ports below 1024
 * Stopped by: port_privilege (defence/port_privilege)
 *
 * Ports below 1024 are where the real services live (22 ssh, 53 DNS,
 * 80 http). A user who takes one before the service starts (or after it
 * crashes) answers in its name: fake login pages, fake DNS answers.
 *
 * launch(): with sockets owned by uid 1, bind TCP port 80 and UDP port 53.
 *           findings = ports taken.
 *
 * Uses: socket_create / socket_bind (core/socket), ui_*.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/privileged_port_bind/defence/port_privilege/def_port_privilege.h" // PORT_PRIVILEGED
#include "modules/network_management/core/socket/socket.h"                      // socket_*
#include "modules/network_management/core/inet/inet.h"                          // INET_ADDR, NET_E*
#include "modules/ui/core/style/ui.h"                                           // ui_printf, ui_color

#define ATTACKER_UID 1

static int simulate(void) {
    ui_printf("  ports 1-1023 are the system's: ssh 22, DNS 53, http 80\n");
    ui_printf("  a user there could answer in the system's name\n");
    return 1;
}

static int launch(void) {
    static const struct { int type; uint16_t port; const char *what; } tries[2] = {
        { SOCK_STREAM, 80, "TCP 80 (http)" }, { SOCK_DGRAM, 53, "UDP 53 (dns)" },
    };
    int taken = 0;
    for (int i = 0; i < 2; i++) {
        socket_t *s;
        if (socket_create(AF_INET, tries[i].type, 0, &s) < 0)
            continue;
        s->uid = ATTACKER_UID;                  // a socket user 1 made
        sockaddr_t a;
        uint32_t len = sizeof(a);
        socket_inet_addr(&a, &len, 0, tries[i].port);
        int r = socket_bind(s, &a, len);
        ui_printf("  uid %d binds %-14s -> %s\n", ATTACKER_UID, tries[i].what,
                  r == 0 ? "TAKEN" : r == -NET_EACCES ? "refused (EACCES)" : "busy");
        taken += r == 0;
        socket_close(s);
    }
    ui_color(taken ? UI_BAD : UI_GOOD);
    ui_printf("  %s\n", taken ? "a user owns a system port" : "only CAP_NET_BIND may take ports below 1024");
    ui_reset();
    return taken;
}

static int cleanup(void) {
    return 0;
}

static const attack_t attack = {
    "privileged_port_bind",
    "as a normal user, take port 80 or 53 and pose as the system's web or DNS server",
    INV_PRIVILEGE, SEV_MEDIUM,
    simulate, launch, cleanup,
    "port_privilege"
};

void atk_privileged_port_bind_register(void) { registry_add_attack(&attack); }

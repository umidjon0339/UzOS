/*
 * atk_sockopt_overflow.c - ATTACK sockopt_overflow: setsockopt() with a lying length
 *
 * Breaks:     integrity - the kernel reads an option only as far as the caller gave it
 * Stopped by: optlen_check (defence/optlen_check)
 *
 * setsockopt(fd, level, name, value, len): the kernel copies 'len' bytes
 * from the program. Code that reads an int (4 bytes) or a timeval (8)
 * from a 1-byte buffer reads past it: past the user's buffer, into
 * whatever lies next (CVE-2016-4998 and many like it).
 *
 * launch(): set SO_RCVBUF with len 1, SO_RCVTIMEO with len 4, SO_REUSEADDR
 *           with len 0. findings = accepted.
 *
 * Uses: socket_create / socket_setopt (core/socket), ui_*.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/sockopt_overflow/defence/optlen_check/def_optlen_check.h" // def_optlen_ok
#include "modules/network_management/core/socket/socket.h"                      // socket_*
#include "modules/network_management/core/inet/inet.h"                          // INET_ADDR, NET_E*
#include "modules/ui/core/style/ui.h"                                           // ui_printf, ui_color

static int simulate(void) {
    ui_printf("  an int option needs 4 bytes; a timeval needs 8\n");
    ui_printf("  a shorter length must be refused, not read past\n");
    return 1;
}

static int launch(void) {
    socket_t *s;
    if (socket_create(AF_INET, SOCK_DGRAM, 0, &s) < 0)
        return 0;
    static const struct { int name; uint32_t len; const char *what; } tries[3] = {
        { SO_RCVBUF, 1, "SO_RCVBUF,   len 1" }, { SO_RCVTIMEO, 4, "SO_RCVTIMEO, len 4" },
        { SO_REUSEADDR, 0, "SO_REUSEADDR, len 0" },
    };
    uint8_t value[8] = { 0x10, 0, 0, 0, 0, 0, 0, 0 };
    int accepted = 0;
    for (int i = 0; i < 3; i++) {
        int r = socket_setopt(s, SOL_SOCKET, tries[i].name, value, tries[i].len);
        ui_printf("  %-20s -> %s\n", tries[i].what, r == 0 ? "ACCEPTED" : "EINVAL");
        accepted += r == 0;
    }
    socket_close(s);
    ui_color(accepted ? UI_BAD : UI_GOOD);
    ui_printf("  %s\n", accepted ? "a short value was read past its end" : "every short length was refused");
    ui_reset();
    return accepted;
}

static int cleanup(void) {
    return 0;
}

static const attack_t attack = {
    "sockopt_overflow",
    "setsockopt() with an option length shorter than the option",
    INV_INTEGRITY, SEV_HIGH,
    simulate, launch, cleanup,
    "optlen_check"
};

void atk_sockopt_overflow_register(void) { registry_add_attack(&attack); }

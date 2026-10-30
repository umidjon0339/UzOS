/*
 * atk_sockaddr_leak.c - ATTACK sockaddr_leak: read leftover kernel bytes out of a sockaddr
 *
 * Breaks:     disclosure - an address the kernel hands out carries no leftover kernel memory
 * Stopped by: sockaddr_zero (defence/sockaddr_zero)
 *
 * sockaddr_in has 8 bytes of padding (sin_zero). A kernel that builds
 * the address on its stack and fills only family, port and address
 * copies whatever was in the other 8 bytes to the program: pieces of
 * old kernel data, pointers that defeat KASLR (many CVEs, e.g. 2013).
 *
 * launch(): getsockname() into a buffer full of 0xAA; look at the padding.
 *           findings = padding bytes that are not 0.
 *
 * Uses: socket_* (core/socket), ui_*.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/sockaddr_leak/defence/sockaddr_zero/def_sockaddr_zero.h" // def_sockaddr_wipe
#include "modules/network_management/core/socket/socket.h"                      // socket_*
#include "modules/network_management/core/inet/inet.h"                          // INET_ADDR, NET_E*
#include "lib/string/string.h"                                                  // memset
#include "modules/ui/core/style/ui.h"                                           // ui_printf, ui_color

static int simulate(void) {
    ui_printf("  sockaddr_in: family 2, port 2, address 4, then 8 bytes of padding\n");
    ui_printf("  unwiped padding hands old kernel stack bytes to the program\n");
    return 1;
}

static int launch(void) {
    socket_t *s;
    if (socket_create(AF_INET, SOCK_DGRAM, 0, &s) < 0)
        return 0;
    sockaddr_t a;
    uint32_t len = sizeof(a);
    socket_inet_addr(&a, &len, 0, 0);
    socket_bind(s, &a, len);                    // any free port
    memset(&a, 0xAA, sizeof(a));
    len = sizeof(a);
    socket_getname(s, 0, &a, &len);
    const sockaddr_in_t *in = (const sockaddr_in_t *)&a;
    int dirty = 0;
    ui_printf("  padding:");
    for (int i = 0; i < 8; i++) {
        ui_printf(" %02x", in->zero[i]);
        dirty += in->zero[i] != 0;
    }
    ui_printf("  (length %u)\n", len);
    socket_close(s);
    ui_color(dirty ? UI_BAD : UI_GOOD);
    ui_printf("  %s\n", dirty ? "bytes that are not ours came back" : "all zero: nothing leaked");
    ui_reset();
    return dirty;
}

static int cleanup(void) {
    return 0;
}

static const attack_t attack = {
    "sockaddr_leak",
    "read the padding of an address getsockname() returns, hoping for old kernel data",
    INV_DISCLOSURE, SEV_MEDIUM,
    simulate, launch, cleanup,
    "sockaddr_zero"
};

void atk_sockaddr_leak_register(void) { registry_add_attack(&attack); }

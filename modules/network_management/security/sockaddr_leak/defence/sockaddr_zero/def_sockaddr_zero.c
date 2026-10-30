/*
 * def_sockaddr_zero.c - DEFENCE sockaddr_zero: wipe an address before filling it.
 *
 * Stops:  sockaddr_leak (security/sockaddr_leak)
 * Called: socket_inet_addr() (core/socket), for getsockname, getpeername,
 *         accept and recvfrom.
 * Rule:   the whole sockaddr is zeroed first, then filled.
 * Why:    padding and unused fields are where kernel leaks hide.
 *
 * Uses: memset (lib/string).
 */

#include "modules/network_management/security/sockaddr_leak/defence/sockaddr_zero/def_sockaddr_zero.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence
#include "lib/string/string.h"                                                  // memset

void def_sockaddr_wipe(void *addr, uint32_t size) {
    memset(addr, 0, size);
}

static int selftest(void) {
    uint8_t b[16];
    memset(b, 0xAA, sizeof(b));
    def_sockaddr_wipe(b, sizeof(b));
    for (int i = 0; i < 16; i++)
        if (b[i])
            return 0;
    return 1;
}

static const defence_t defence = {
    "sockaddr_zero",
    "every address handed to a program is wiped before it is filled in",
    "sockaddr_leak",
    selftest
};

void def_sockaddr_zero_register(void) { registry_add_defence(&defence); }

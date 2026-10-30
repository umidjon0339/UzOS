/*
 * def_reassembly_bounds.c - DEFENCE reassembly_bounds: no fragment writes past the reassembly buffer.
 *
 * Stops:  ping_of_death (security/ping_of_death)
 * Called: ipv4_reassemble() (core/ipv4/fragment.c), before it copies.
 * Rule:   offset + length <= the buffer (IPV4_REASM_MAX). Otherwise the
 *         whole datagram is thrown away.
 * Why:    the 1996 ping of death: the sum was never checked.
 *
 * Uses: audit_log (security_management).
 */

#include "modules/network_management/security/ping_of_death/defence/reassembly_bounds/def_reassembly_bounds.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                       // audit_log

static int fits(uint32_t offset, uint32_t len, uint32_t size) {
    return offset <= size && len <= size - offset;      // no overflow in the sum either
}

int def_reasm_fits(uint32_t offset, uint32_t len, uint32_t size) {
    if (fits(offset, len, size))
        return 1;
    audit_log("reassembly_bounds", "fragment refused: it ends past the datagram (ping of death?)");
    return 0;
}

static int selftest(void) {
    if (!fits(0, 1480, 1580)) return 0;
    if (!fits(1480, 100, 1580)) return 0;
    if (fits(1480, 101, 1580)) return 0;
    if (fits(65528, 100, 1580)) return 0;
    if (fits(8, 0xFFFFFFFC, 1580)) return 0;            // a sum that would wrap
    return 1;
}

static const defence_t defence = {
    "reassembly_bounds",
    "a fragment must end inside the reassembly buffer",
    "ping_of_death",
    selftest
};

void def_reassembly_bounds_register(void) { registry_add_defence(&defence); }

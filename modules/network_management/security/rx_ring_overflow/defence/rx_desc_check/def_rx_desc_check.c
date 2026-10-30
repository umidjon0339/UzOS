/*
 * def_rx_desc_check.c - DEFENCE rx_desc_check (ILLUSTRATION): never trust a descriptor's length.
 *
 * Stops:  rx_ring_overflow (security/rx_ring_overflow)
 * Called: receive_all() (core/e1000), for every descriptor the card finished.
 * Rule:   the frame must be complete (DD and EOP set), carry no error
 *         bits, and its length must lie between 14 and the buffer size.
 * Why:    the length comes from a device. A driver that copies what a
 *         device says is one bug (or one malicious device) away from
 *         reading or writing outside its buffer.
 *
 * Uses: audit_log (security_management).
 */

#include "modules/network_management/security/rx_ring_overflow/defence/rx_desc_check/def_rx_desc_check.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                       // audit_log

static int ok(uint8_t status, uint8_t errors, uint32_t len, uint32_t buffer_size) {
    return (status & 0x3) == 0x3 && errors == 0 && len >= 14 && len <= buffer_size;
}

int def_rx_desc_ok(uint8_t status, uint8_t errors, uint32_t len, uint32_t buffer_size) {
    if (ok(status, errors, len, buffer_size))
        return 1;
    audit_log("rx_desc_check", "e1000: dropped a frame its descriptor could not vouch for");
    return 0;
}

static int selftest(void) {
    if (!ok(0x3, 0, 60, 2048)) return 0;
    if (!ok(0x3, 0, 2048, 2048)) return 0;
    if (ok(0x3, 0, 2049, 2048)) return 0;
    if (ok(0x1, 0, 60, 2048)) return 0;
    if (ok(0x3, 1, 60, 2048)) return 0;
    if (ok(0x3, 0, 4, 2048)) return 0;
    return 1;
}

static const defence_t defence = {
    "rx_desc_check",
    "the driver takes only whole, error-free frames that fit their buffer",
    "rx_ring_overflow",
    selftest
};

void def_rx_desc_check_register(void) { registry_add_defence(&defence); }

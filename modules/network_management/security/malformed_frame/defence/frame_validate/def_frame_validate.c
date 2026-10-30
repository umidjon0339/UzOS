/*
 * def_frame_validate.c - DEFENCE frame_validate: only well-formed Ethernet II frames go up.
 *
 * Stops:  malformed_frame (security/malformed_frame)
 * Called: ethernet_receive() (core/ethernet), before anything reads the header.
 * Rule:   14 <= length <= 1514 (header + 1500; the card strips the CRC),
 *         and the type field >= 0x0600 (smaller values are 802.3 lengths).
 * Why:    a length check before every read is what keeps a parser
 *         inside its buffer.
 *
 * Uses: audit_log (security_management).
 */

#include "modules/network_management/security/malformed_frame/defence/frame_validate/def_frame_validate.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                       // audit_log

static int ok(uint32_t len, uint16_t type) {
    return len >= FRAME_MIN && len <= FRAME_MAX && type >= 0x0600;
}

int def_frame_ok(uint32_t len, uint16_t type) {
    if (ok(len, type))
        return 1;
    audit_log("frame_validate", "dropped a malformed Ethernet frame");
    return 0;
}

static int selftest(void) {
    if (!ok(60, 0x0800)) return 0;      // a normal frame
    if (!ok(1514, 0x0806)) return 0;    // the biggest
    if (ok(13, 0x0800)) return 0;       // too short
    if (ok(1515, 0x0800)) return 0;     // too long
    if (ok(60, 0x0040)) return 0;       // 802.3 length, not a type
    return 1;
}

static const defence_t defence = {
    "frame_validate",
    "a frame must be 14-1514 bytes with a real EtherType",
    "malformed_frame",
    selftest
};

void def_frame_validate_register(void) { registry_add_defence(&defence); }

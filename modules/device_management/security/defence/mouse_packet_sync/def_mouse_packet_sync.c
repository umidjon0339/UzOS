/*
 * def_mouse_packet_sync.c - DEFENCE mouse_packet_sync: stay in step with
 * the mouse's packets.
 *
 * Stops:  mouse_desync (security/attack/mouse_desync)
 * Called: the mouse packet decoder (mouse/).
 * Rules:  1. a packet's first byte always has bit 3 set; if a byte at
 *            the start of a packet doesn't, drop it and wait
 *         2. a packet whose overflow bits (6, 7) are set is dropped
 * Why:    the mouse sends a stream of bytes with no markers. If one byte
 *         is lost (or an extra one arrives), every later packet is read
 *         shifted by one: movements become button presses and the
 *         pointer jumps around. Bit 3 is the only "this is the start"
 *         sign the protocol has, so we use it to find our step again.
 */

#include "def_mouse_packet_sync.h"
#include "registry.h"
#include "audit.h"

#define ALWAYS_ONE   0x08       // bit 3 of the first byte
#define OVERFLOW     0xC0       // bits 6 and 7: x / y overflowed

// The rules, as pure functions so the selftest can try them.
static int is_packet_start(uint8_t byte)   { return (byte & ALWAYS_ONE) != 0; }
static int has_overflow(uint8_t first)     { return (first & OVERFLOW) != 0; }

int def_mouse_sync_ok(uint8_t byte) {
    if (is_packet_start(byte))
        return 1;
    audit_log("mouse_packet_sync", "mouse byte dropped: packet out of step");
    return 0;
}

int def_mouse_packet_ok(const uint8_t *packet) {
    if (!has_overflow(packet[0]))
        return 1;
    audit_log("mouse_packet_sync", "mouse packet dropped: movement overflowed");
    return 0;
}

static int selftest(void) {
    if (!is_packet_start(0x08)) return 0;       // no buttons, no movement
    if (!is_packet_start(0x39)) return 0;       // left button, moving left/down
    if (is_packet_start(0x00)) return 0;        // a dx/dy byte, not a start
    if (is_packet_start(0x37)) return 0;        // bit 3 clear
    if (has_overflow(0x08)) return 0;
    if (!has_overflow(0x48)) return 0;          // x overflowed
    if (!has_overflow(0x88)) return 0;          // y overflowed
    return 1;
}

static const defence_t defence = {
    "mouse_packet_sync",
    "resync on bit 3 of the first byte; drop overflowed packets",
    "mouse_desync",
    selftest
};

void def_mouse_packet_sync_register(void) {
    registry_add_defence(&defence);
}

/*
 * atk_mouse_desync.c - ATTACK mouse_desync: knock the decoder out of step.
 *
 * Breaks:     consistency - each byte is read as what it really is
 * Stopped by: mouse_packet_sync (defence/mouse_packet_sync)
 *
 * The mouse stream has no start markers. One stray byte makes a naive
 * decoder read every following packet shifted: movement becomes clicks.
 *
 * simulate(): show how a naive decoder would misread the bytes.
 * launch():   feed the real decoder (interrupts off) one stray byte, then
 *             a real packet "move right 10, left button down", then an
 *             overflowed packet. Check what came out; put the pointer back.
 *
 * Uses: mouse_handle_byte / mouse_get_state / mouse_set_state (mouse/).
 */

#include "registry.h"
#include "audit.h"
#include "mouse.h"
#include "cpu.h"
#include "vga.h"

#define STRAY       0x00        // a byte with bit 3 clear: cannot start a packet
#define LEFT_BUTTON 0x09        // flags: bit 3 (always 1) + left button
#define OVERFLOWED  0x48        // flags: bit 3 + "x overflowed"

static void send(const uint8_t *bytes, int n) {
    for (int i = 0; i < n; i++)
        mouse_handle_byte(bytes[i]);
    if (mouse_packet_size() == 4 && n == 3)
        mouse_handle_byte(0);   // wheel byte
}

static int simulate(void) {
    vga_print_str("  sent    : [stray 00] [flags 09, dx 0A, dy 00]\n");
    vga_print_str("  naive   : flags=00 dx=09 dy=0A -> no click, moves 9 right, 10 up\n");
    vga_print_str("            then 00 starts the next packet... every packet is off by one\n");
    vga_print_str("  in sync : flags=09 dx=0A dy=00 -> left click, moves 10 right\n");
    return 1;
}

static int launch(void) {
    mouse_state_t saved, after;
    uint32_t denied = audit_total();
    uint8_t stray = STRAY;
    uint8_t click_right[3] = { LEFT_BUTTON, 10, 0 };
    uint8_t overflow[3]    = { OVERFLOWED, 0xFF, 0 };

    uint32_t irq = cpu_irq_save();
    mouse_get_state(&saved);
    mouse_reset_decoder();

    send(&stray, 1);                        // the stray byte
    send(click_right, 3);                   // a real packet right after it
    mouse_get_state(&after);
    int expected_x = saved.x + 10 > MOUSE_SCREEN_MAX_X ? MOUSE_SCREEN_MAX_X : saved.x + 10;
    int ok = after.left == 1 && after.x == expected_x;
    vga_print_str("  stray byte, then 'left click + 10 right' -> ");
    vga_print_str(ok ? "read correctly\n" : "MISREAD (out of step)\n");

    int before_x = after.x;
    send(overflow, 3);                      // movement too big to report
    mouse_get_state(&after);
    vga_print_str("  overflowed packet -> ");
    vga_print_str(after.x == before_x ? "dropped\n" : "APPLIED\n");

    mouse_set_state(&saved);
    mouse_reset_decoder();
    cpu_irq_restore(irq);
    return audit_total() - denied;
}

static const attack_t attack = {
    "mouse_desync",
    "one stray byte shifts every following mouse packet",
    INV_CONSISTENCY, SEV_MEDIUM,
    simulate, launch, 0,        // launch restores the pointer itself
    "mouse_packet_sync"
};

void atk_mouse_desync_register(void) {
    registry_add_attack(&attack);
}

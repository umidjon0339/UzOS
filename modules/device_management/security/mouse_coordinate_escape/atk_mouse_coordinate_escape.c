/*
 * atk_mouse_coordinate_escape.c - ATTACK mouse_coordinate_escape: move the
 * pointer off the screen.
 *
 * Breaks:     consistency - the pointer is always a real screen position
 * Stopped by: mouse_bounds (defence/mouse_bounds)
 *
 * The mouse reports only movement (up to 255 per packet). Keep pushing
 * left at x = 0 and the sum goes negative; the GUI then uses a negative
 * number as a pixel index.
 *
 * simulate(): say where 10 big moves in each direction would end up.
 * launch():   feed those packets to the real decoder (interrupts off),
 *             check the pointer after each burst, then put it back.
 *
 * Uses: mouse_handle_byte / mouse_get_state / mouse_set_state and
 *       mouse_max_x / mouse_max_y: the current screen's edges (mouse/),
 *       cpu_irq_save (arch/x86/cpu).
 */

#include "registry.h"
#include "mouse.h"
#include "cpu.h"
#include "vga.h"

#define PUSHES  10              // 10 x 255 = 2550: past any edge (1920 is the widest)
#define STEP    255

// Feed one movement packet to the decoder, exactly as the mouse would.
static void send_move(int dx, int dy) {
    uint8_t flags = 0x08 | (dx < 0 ? 0x10 : 0) | (dy < 0 ? 0x20 : 0);
    mouse_handle_byte(flags);
    mouse_handle_byte((uint8_t)dx);
    mouse_handle_byte((uint8_t)dy);
    if (mouse_packet_size() == 4)
        mouse_handle_byte(0);               // no wheel movement
}

static const struct { const char *name; int dx, dy; } directions[] = {
    { "left ", -STEP, 0 }, { "right",  STEP, 0 },
    { "up   ", 0,  STEP }, { "down ", 0, -STEP },   // PS/2: +dy is up
};

static int simulate(void) {
    mouse_state_t m;
    mouse_get_state(&m);
    vga_print_str("  pointer now at ");
    vga_print_int(m.x);
    vga_print_str(", ");
    vga_print_int(m.y);
    vga_print_str("\n  10 pushes left without a check would reach x = ");
    vga_print_int(m.x - PUSHES * STEP);
    vga_print_str("\n");
    return 4;
}

static int launch(void) {
    int escaped = 0;
    mouse_state_t saved, now;

    uint32_t irq = cpu_irq_save();          // no real mouse bytes in between
    mouse_get_state(&saved);
    mouse_reset_decoder();

    for (int d = 0; d < 4; d++) {
        for (int i = 0; i < PUSHES; i++)
            send_move(directions[d].dx, directions[d].dy);
        mouse_get_state(&now);

        int off = now.x < 0 || now.x > mouse_max_x() ||
                  now.y < 0 || now.y > mouse_max_y();
        escaped += off;
        vga_print_str("  10 x 255 ");
        vga_print_str(directions[d].name);
        vga_print_str(" -> ");
        vga_print_int(now.x);
        vga_print_str(", ");
        vga_print_int(now.y);
        vga_print_str(off ? "  OFF SCREEN\n" : "  (on screen)\n");
    }

    mouse_set_state(&saved);                // put the pointer back
    mouse_reset_decoder();
    cpu_irq_restore(irq);
    return escaped;
}

static const attack_t attack = {
    "mouse_coordinate_escape",
    "big mouse movements push the pointer outside the screen",
    INV_CONSISTENCY, SEV_MEDIUM,
    simulate, launch, 0,        // launch restores the pointer itself
    "mouse_bounds"
};

void atk_mouse_coordinate_escape_register(void) {
    registry_add_attack(&attack);
}

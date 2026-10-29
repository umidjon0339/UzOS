/*
 * atk_window_exhaustion.c - ATTACK window_exhaustion: open windows until
 *                           the table is full.
 *
 * Breaks:     resource - window slots (and the heap behind them) are finite
 * Stopped by: window_quota (defence/window_quota)
 *
 * The window manager keeps MAX_WINDOWS fixed slots, each with a canvas
 * on the heap. A program that opens window after window would, without a
 * cap, take every slot and drain the heap so nothing else can open one.
 * The cap refuses a new window once all slots are used.
 *
 * simulate(): explain the fixed slot table.
 * launch():   really ask for MAX_WINDOWS + 4 small windows; the extras
 *             are refused. Count how many were created.
 * cleanup():  destroy every window this attack opened.
 *
 * Uses: wm_create_window / wm_destroy_window (gui/wm/), window_quota, vga.
 */

#include "registry.h"
#include "audit.h"
#include "window_manager.h"
#include "vga.h"

#define ATTEMPTS (MAX_WINDOWS + 4)

static int ids[ATTEMPTS];
static int made;

static int simulate(void) {
    vga_print_str("  the window manager has MAX_WINDOWS fixed slots\n");
    vga_print_str("  each window holds a canvas on the heap\n");
    vga_print_str("  past the cap a new window is refused, not squeezed in\n");
    return 1;
}

static int launch(void) {
    uint32_t before = audit_total();
    made = 0;
    for (int i = 0; i < ATTEMPTS; i++) {
        int id = wm_create_window(0, 0, 10, 10, "atk");   // tiny windows
        if (id >= 0)
            ids[made++] = id;
    }
    uint32_t refused = audit_total() - before;
    vga_print_str("  windows created: "); vga_print_int(made);
    vga_print_str(" (cap "); vga_print_int(MAX_WINDOWS);
    vga_print_str("), refused: "); vga_print_int((int)refused);
    vga_print_str("\n");
    return made > MAX_WINDOWS ? made - MAX_WINDOWS : 0;    // over the cap?
}

static int cleanup(void) {
    for (int i = 0; i < made; i++)
        wm_destroy_window(ids[i]);
    int n = made;
    made = 0;
    return n;
}

static const attack_t attack = {
    "window_exhaustion",
    "open window after window until the slot table is full",
    INV_RESOURCE, SEV_MEDIUM,
    simulate, launch, cleanup,
    "window_quota"
};

void atk_window_exhaustion_register(void) { registry_add_attack(&attack); }

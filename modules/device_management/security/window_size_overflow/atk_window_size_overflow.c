/*
 * atk_window_size_overflow.c - ATTACK window_size_overflow: ask for a
 *                              window too big to allocate safely.
 *
 * Breaks:     trust - a size is used in allocation math; check it first
 * Stopped by: window_bounds (defence/window_bounds)
 *
 * wm_create_window(x, y, w, h, title) allocates its canvas with
 * kmalloc(w * h * 4). A size like 100000 x 100000 overflows that
 * multiplication - kmalloc hands back a tiny buffer, and the first draw
 * runs far past it - or simply drains the heap. The guard refuses any
 * window that is zero-sized or larger than the screen.
 *
 * simulate(): show what each bad size does to the kmalloc math.
 * launch():   call the real wm_create_window() with them; each is
 *             refused before kmalloc. If one slipped through, undo it.
 * cleanup():  nothing should remain, but destroy anything that did.
 *
 * Uses: wm_create_window / wm_destroy_window (gui/wm/), window_bounds, vga.
 */

#include "registry.h"
#include "audit.h"
#include "window_manager.h"
#include "vga.h"

static const struct { int w, h; const char *effect; } bad[] = {
    { 0,      100,    "zero width: a zero-size allocation" },
    { 100000, 100000, "10 Gpx * 4 B: kmalloc size overflows 32 bits" },
    { 2000,   2000,   "bigger than the screen" },
};
#define BADN (sizeof(bad) / sizeof(bad[0]))

static int simulate(void) {
    for (unsigned i = 0; i < BADN; i++) {
        vga_print_str("  "); vga_print_str(bad[i].effect); vga_print_str("\n");
    }
    return 1;
}

static int launch(void) {
    uint32_t before = audit_total();
    int leaked = 0;
    for (unsigned i = 0; i < BADN; i++) {
        int id = wm_create_window(0, 0, bad[i].w, bad[i].h, "atk");
        if (id >= 0) { wm_destroy_window(id); leaked++; }   // a breach: undo it
    }
    uint32_t refused = audit_total() - before;
    vga_print_str("  giant windows refused: "); vga_print_int((int)refused);
    vga_print_str(" of "); vga_print_int(BADN); vga_print_str("\n");
    return leaked;      // 0 = none were allocated
}

static int cleanup(void) { return 0; }

static const attack_t attack = {
    "window_size_overflow",
    "ask for a window so large that kmalloc(w*h*4) overflows",
    INV_TRUST, SEV_MEDIUM,
    simulate, launch, cleanup,
    "window_bounds"
};

void atk_window_size_overflow_register(void) { registry_add_attack(&attack); }

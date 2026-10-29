/*
 * atk_bga_mode_overflow.c - ATTACK bga_mode_overflow: an insane video mode.
 *
 * Breaks:     trust - the video size is used in memory math; check it
 * Stopped by: mode_bounds (defence/mode_bounds)
 *
 * bga_set_video_mode(w, h, bpp) tells the graphics card to switch mode.
 * Other code then sizes the framebuffer as w * h * bpp/8 and draws into
 * it. A mode like 65535 x 65535 x 32 overflows that multiplication; a
 * zero width makes a zero-size buffer. Either way later draws run off
 * the end. The guard refuses any size outside the card's real range.
 *
 * simulate(): show what each bad mode would do to the framebuffer math.
 * launch():   call the real bga_set_video_mode() with them; each is
 *             refused before any card register is written.
 * cleanup():  nothing (nothing was changed).
 *
 * Uses: bga_set_video_mode (gui/bga/), mode_bounds, vga.
 */

#include "registry.h"
#include "audit.h"
#include "bga.h"
#include "vga.h"

static const struct { uint16_t w, h, bpp; const char *effect; } bad[] = {
    { 0,     768,   32, "zero width: a framebuffer of no size" },
    { 65535, 65535, 32, "4 Gpx * 4 B: framebuffer size overflows 32 bits" },
    { 1024,  768,   7,  "7 bpp: not a real colour depth" },
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
    for (unsigned i = 0; i < BADN; i++)
        bga_set_video_mode(bad[i].w, bad[i].h, bad[i].bpp);  // refused first
    uint32_t refused = audit_total() - before;
    vga_print_str("  bad video modes refused: "); vga_print_int((int)refused);
    vga_print_str(" of "); vga_print_int(BADN); vga_print_str("\n");
    return (int)(BADN - refused);
}

static int cleanup(void) { return 0; }

static const attack_t attack = {
    "bga_mode_overflow",
    "an out-of-range video resolution to overflow framebuffer math",
    INV_TRUST, SEV_MEDIUM,
    simulate, launch, cleanup,
    "mode_bounds"
};

void atk_bga_mode_overflow_register(void) { registry_add_attack(&attack); }

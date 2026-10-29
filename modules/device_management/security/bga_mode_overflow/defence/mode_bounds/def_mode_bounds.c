/*
 * def_mode_bounds.c - DEFENCE mode_bounds: only sane video modes.
 *
 * Stops:  bga_mode_overflow (security/bga_mode_overflow)
 * Called: bga_set_video_mode() (gui/bga/bga.c), before it programs the
 *         card registers.
 * Rule:   0 < width <= BGA_MAX_WIDTH, 0 < height <= BGA_MAX_HEIGHT,
 *         bpp in {15, 16, 24, 32}.
 * Why:    other code sizes the framebuffer as width * height * bpp/8 and
 *         maps/draws it. A huge or zero mode would overflow that math or
 *         make a zero-size buffer, and later draws would run off its end.
 */

#include "def_mode_bounds.h"
#include "registry.h"
#include "audit.h"

static int bpp_ok(uint16_t bpp) {
    return bpp == 15 || bpp == 16 || bpp == 24 || bpp == 32;
}

static int mode_ok(uint16_t w, uint16_t h, uint16_t bpp) {
    return w > 0 && h > 0 && w <= BGA_MAX_WIDTH && h <= BGA_MAX_HEIGHT && bpp_ok(bpp);
}

int def_bga_mode_ok(uint16_t width, uint16_t height, uint16_t bpp) {
    if (mode_ok(width, height, bpp))
        return 1;
    audit_log("mode_bounds", "video mode refused: size out of range or odd depth");
    return 0;
}

static int selftest(void) {
    if (!mode_ok(1024, 768, 32)) return 0;        // a normal mode
    if (!mode_ok(640, 480, 16)) return 0;
    if (mode_ok(0, 768, 32)) return 0;            // zero width
    if (mode_ok(1024, 0, 32)) return 0;           // zero height
    if (mode_ok(65535, 65535, 32)) return 0;      // framebuffer math overflows
    if (mode_ok(1024, 768, 7)) return 0;          // not a real depth
    if (mode_ok(BGA_MAX_WIDTH + 1, 1080, 32)) return 0;
    return 1;
}

static const defence_t defence = {
    "mode_bounds",
    "a video mode must fit the card's size and be a real colour depth",
    "bga_mode_overflow",
    selftest
};

void def_mode_bounds_register(void) { registry_add_defence(&defence); }

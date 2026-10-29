/*
 * def_contrast_check.c - DEFENCE contrast_check: text can always be read.
 *
 * Stops:  invisible_text (security/invisible_text)
 * Called: palette_use() (vga/palette) for every theme, and the shell's
 *         `color` command, before the colours are used.
 * Rule:   |brightness(fg) - brightness(bg)| >= MIN_CONTRAST (48 of 255)
 *         brightness = (299 R + 587 G + 114 B) / 1000   (the eye sees
 *         green as brighter than red, and red as brighter than blue)
 * Why:    text in the same colour as its background is still there, but
 *         nobody can see it. An attacker can hide output that way: a
 *         warning, a "kill" that happened, or a fake prompt the user
 *         types a password into. The screen must never lie by omission.
 */

#include "def_contrast_check.h"
#include "registry.h"
#include "audit.h"

int def_brightness(uint32_t rgb) {
    int r = rgb >> 16 & 0xFF, g = rgb >> 8 & 0xFF, b = rgb & 0xFF;
    return (299 * r + 587 * g + 114 * b) / 1000;
}

// The whole rule, as a pure function so the selftest can try it.
static int readable(uint32_t fg, uint32_t bg) {
    int diff = def_brightness(fg) - def_brightness(bg);
    return diff >= MIN_CONTRAST || -diff >= MIN_CONTRAST;
}

int def_contrast_ok(uint32_t fg, uint32_t bg, const char *what) {
    if (readable(fg, bg))
        return 1;
    audit_log("contrast_check", what);
    return 0;
}

static int selftest(void) {
    if (!readable(0xFFFFFF, 0x000000)) return 0;    // white on black
    if (!readable(0x000000, 0xFFFFFF)) return 0;    // black on white
    if (!readable(0xAAAAAA, 0x000000)) return 0;    // classic grey text
    if (readable(0x000000, 0x000000)) return 0;     // the same colour
    if (readable(0x1A1B26, 0x1A1B26)) return 0;
    if (readable(0x0000AA, 0x000000)) return 0;     // dark blue on black
    if (readable(0x202020, 0x101010)) return 0;     // two near-blacks
    return 1;
}

static const defence_t defence = {
    "contrast_check",
    "refuse text colours that are invisible on their background",
    "invisible_text",
    selftest
};

void def_contrast_check_register(void) {
    registry_add_defence(&defence);
}

/*
 * atk_invisible_text.c - ATTACK invisible_text: hide output in plain sight.
 *
 * Breaks:     trust - what the screen shows is what really happened
 * Stopped by: contrast_check (security/defence/contrast_check)
 *
 * Text drawn in its background colour is still printed, but nobody sees
 * it. A theme where "white" equals "black" hides every message; one
 * where only white equals black hides exactly the important ones.
 *
 * simulate(): explain the trick.
 * launch():   try to install two such themes with the real palette_use().
 *             Both must be refused and the current theme must stay.
 *
 * Uses: palette_use / palette_current (vga/palette).
 */

#include "registry.h"
#include "audit.h"
#include "palette.h"
#include "vga.h"

#define BG 0x1A1B26

static const theme_t all_hidden = {
    "evil-all", "every colour is the background", {
        BG, BG, BG, BG, BG, BG, BG, BG, BG, BG, BG, BG, BG, BG, BG, BG },
    BG, BG };

static const theme_t white_hidden = {
    "evil-white", "only 'white' text is hidden", {
        BG, 0x0099B5, 0x1EB53A, 0x2AC3DE, 0xCE1126, 0x9D7CD8, 0xFF9E64, 0xC0CAF5,
        0x565F89, 0x7AA2F7, 0x9ECE6A, 0x7DCFFF, 0xF7768E, 0xBB9AF7, 0xE0AF68, BG + 0x010101 },
    BG, BG };

static int simulate(void) {
    vga_print_str("  a theme maps colour 15 ('white') to the background colour\n");
    vga_print_str("  -> every message printed in white is on screen but unreadable,\n");
    vga_print_str("     e.g. 'process 3 killed' or a fake 'password:' prompt\n");
    return 1;
}

static int launch(void) {
    uint32_t denied = audit_total();
    const char *before = palette_current()->name;

    vga_print_str("  install theme where all colours = background -> ");
    vga_print_str(palette_use(&all_hidden) < 0 ? "refused\n" : "INSTALLED\n");
    vga_print_str("  install theme where only white = background  -> ");
    vga_print_str(palette_use(&white_hidden) < 0 ? "refused\n" : "INSTALLED\n");

    vga_print_str("  theme now: ");
    vga_print_str(palette_current()->name);
    vga_print_str(palette_current()->name == before ? " (unchanged)\n" : " (CHANGED)\n");
    return audit_total() - denied;
}

static const attack_t attack = {
    "invisible_text",
    "install colours that hide text by matching the background",
    INV_TRUST, SEV_MEDIUM,
    simulate, launch, 0,        // refused: the theme never changed
    "contrast_check"
};

void atk_invisible_text_register(void) {
    registry_add_attack(&attack);
}

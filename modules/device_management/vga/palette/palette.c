/*
 * palette.c - The themes, and programming the VGA DAC.
 *
 * DAC programming: write the first colour index to port 0x3C8, then
 * three bytes (red, green, blue) per colour to port 0x3C9. The index
 * moves on by itself. The DAC only has 6 bits per channel (0-63), so
 * every 8-bit value is shifted right by 2.
 *
 * The text driver sets the attribute controller so colour number i uses
 * DAC entry i (see vga_text.c), so entries 0-15 are all we need.
 *
 * Uses: outb (arch/x86/io), def_contrast_ok (defence contrast_check).
 */

#include "palette.h"
#include "io.h"
#include "string.h"
#include "def_contrast_check.h"

#define DAC_WRITE_INDEX 0x3C8
#define DAC_DATA        0x3C9

//                0 black   1 blue    2 green   3 cyan    4 red     5 magenta 6 brown   7 grey
//                8 dkgrey  9 ltblue  10 ltgrn  11 ltcyan 12 ltred  13 pink   14 yellow 15 white
static const theme_t themes[] = {
    { "uzos", "night blue, accents from the Uzbek flag", {
        0x1A1B26, 0x0099B5, 0x1EB53A, 0x2AC3DE, 0xCE1126, 0x9D7CD8, 0xFF9E64, 0xC0CAF5,
        0x565F89, 0x7AA2F7, 0x9ECE6A, 0x7DCFFF, 0xF7768E, 0xBB9AF7, 0xE0AF68, 0xEEF1FF },
      0x24283B, 0x101119 },
    { "classic", "the original VGA colours", {
        0x000000, 0x0000AA, 0x00AA00, 0x00AAAA, 0xAA0000, 0xAA00AA, 0xAA5500, 0xAAAAAA,
        0x555555, 0x5555FF, 0x55FF55, 0x55FFFF, 0xFF5555, 0xFF55FF, 0xFFFF55, 0xFFFFFF },
      0x000000, 0x000000 },
    { "nord", "cool arctic blues", {
        0x2E3440, 0x5E81AC, 0x7FA36A, 0x88C0D0, 0xBF616A, 0xB48EAD, 0xD08770, 0xD8DEE9,
        0x4C566A, 0x81A1C1, 0xA3BE8C, 0x8FBCBB, 0xE5838C, 0xC9A3C2, 0xEBCB8B, 0xECEFF4 },
      0x3B4252, 0x2E3440 },
    { "dracula", "dark purple with bright colours", {
        0x282A36, 0x6272A4, 0x3BB560, 0x5FB8C9, 0xCC4444, 0x9A72D0, 0xFFB86C, 0xE0E0DA,
        0x6272A4, 0x8BA4F9, 0x50FA7B, 0x8BE9FD, 0xFF5555, 0xFF79C6, 0xF1FA8C, 0xFFFFFF },
      0x343746, 0x1E1F29 },
    { "gruvbox", "warm retro browns", {
        0x282828, 0x458588, 0x98971A, 0x689D6A, 0xCC241D, 0xB16286, 0xD79921, 0xD5C4A1,
        0x928374, 0x83A598, 0xB8BB26, 0x8EC07C, 0xFB4934, 0xD3869B, 0xFABD2F, 0xFBF1C7 },
      0x32302F, 0x1D2021 },
    { "matrix", "green on black, like an old terminal", {
        0x020A02, 0x0A5A1A, 0x1F8F3A, 0x2FAF5A, 0x3F7F3F, 0x2F6F4F, 0x4F8F2F, 0x5FDF7F,
        0x2F5F3F, 0x4FBF6F, 0x7FFF9F, 0x6FEF8F, 0xAFFF7F, 0x8FFFAF, 0xCFFFCF, 0xDFFFDF },
      0x041A08, 0x000500 },
};
#define THEME_COUNT (int)(sizeof(themes) / sizeof(themes[0]))

static theme_t current;             // a copy, so palette_use() can take any theme
static int have_current;

int palette_theme_count(void) {
    return THEME_COUNT;
}

const theme_t *palette_theme(int index) {
    return (index >= 0 && index < THEME_COUNT) ? &themes[index] : 0;
}

const theme_t *palette_current(void) {
    return have_current ? &current : &themes[0];
}

int palette_use(const theme_t *theme) {
    // Normal text is colour 7 and 15 on colour 0: both must be readable.
    if (!def_contrast_ok(theme->rgb[7], theme->rgb[0], "theme: grey text on background") ||
        !def_contrast_ok(theme->rgb[15], theme->rgb[0], "theme: white text on background"))
        return -1;
    current = *theme;
    have_current = 1;
    return 1;
}

int palette_select(const char *name) {
    for (int i = 0; i < THEME_COUNT; i++)
        if (strcmp(themes[i].name, name) == 0)
            return palette_use(&themes[i]);
    return 0;
}

uint32_t palette_rgb(int color) {
    return palette_current()->rgb[color & 0x0F];
}

void palette_apply_vga(void) {
    outb(DAC_WRITE_INDEX, 0);
    for (int i = 0; i < 16; i++) {
        uint32_t rgb = palette_rgb(i);
        outb(DAC_DATA, (rgb >> 16 & 0xFF) >> 2);    // red
        outb(DAC_DATA, (rgb >> 8 & 0xFF) >> 2);     // green
        outb(DAC_DATA, (rgb & 0xFF) >> 2);          // blue
    }
}

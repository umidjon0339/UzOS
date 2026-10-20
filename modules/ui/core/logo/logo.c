/*
 * logo.c - Draw the banner and the flag with code page 437 blocks.
 *
 * The banner is drawn from pictures: each '#' becomes a full block █.
 * Its rows take the colours of the Uzbek flag: blue, white, green.
 *
 * The flag is 8 rows of coloured cells. Half blocks (▄ ▀) paint the
 * thin red stripes: one cell can show two colours, the top half in the
 * background colour and the bottom half in the foreground colour.
 *
 *   row 0-1  blue, with the crescent and stars
 *   row 2    blue on top, red below          ▄ red on blue
 *   row 3-4  white
 *   row 5    red on top, green below         ▀ red on green
 *   row 6-7  green
 *
 * Uses: vga_* (device_management), GLYPH_* icons (font/), ui colours.
 */

#include "logo.h"
#include "ui.h"

#define FULL_BLOCK   '\xDB'     // █
#define LOWER_HALF   '\xDC'     // ▄
#define UPPER_HALF   '\xDF'     // ▀

static const char *const banner[] = {
    "##  ##          ######  ######",
    "##  ##  ######  ##  ##  ##    ",
    "##  ##     ##   ##  ##  ######",
    "##  ##   ##     ##  ##      ##",
    "######  ######  ######  ######",
};
static const enum vga_color banner_colors[] = {
    VGA_COLOR_LIGHT_CYAN, VGA_COLOR_LIGHT_CYAN, VGA_COLOR_WHITE,
    VGA_COLOR_LIGHT_GREEN, VGA_COLOR_LIGHT_GREEN,
};

void logo_print_banner(void) {
    vga_print_str("\n");
    for (int r = 0; r < 5; r++) {
        ui_color(banner_colors[r]);
        vga_print_str("      ");
        for (const char *p = banner[r]; *p; p++)
            vga_put_char(*p == '#' ? FULL_BLOCK : ' ');
        if (r == 1) {
            ui_color(VGA_COLOR_WHITE);
            vga_print_str("    " GLYPH_CRESCENT " " GLYPH_STAR GLYPH_STAR GLYPH_STAR);
        }
        vga_print_str("\n");
    }

    ui_color(UI_ACCENT);
    vga_print_str("\n      Uzbekistan Operating System");
    ui_color(UI_DIM);
    vga_print_str("   v1.0 \xFA i386 \xFA 2026\n\n");   // · between the parts
    ui_reset();
}

// Fill the rest of the flag row with 'c' in fg on bg.
static void fill(int from, char c, enum vga_color fg, enum vga_color bg) {
    vga_set_color(fg, bg);
    for (int i = from; i < LOGO_FLAG_WIDTH; i++)
        vga_put_char(c);
}

void logo_print_flag_row(int row) {
    switch (row) {
    case 0:
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
        vga_print_str(" " GLYPH_CRESCENT "  " GLYPH_STAR " " GLYPH_STAR " " GLYPH_STAR);
        fill(9, ' ', VGA_COLOR_WHITE, VGA_COLOR_BLUE);
        break;
    case 1:
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
        vga_print_str("    " GLYPH_STAR " " GLYPH_STAR " " GLYPH_STAR " " GLYPH_STAR);
        fill(11, ' ', VGA_COLOR_WHITE, VGA_COLOR_BLUE);
        break;
    case 2:  fill(0, LOWER_HALF, VGA_COLOR_RED, VGA_COLOR_BLUE);   break;
    case 3:
    case 4:  fill(0, ' ', VGA_COLOR_WHITE, VGA_COLOR_WHITE);       break;
    case 5:  fill(0, UPPER_HALF, VGA_COLOR_RED, VGA_COLOR_GREEN);  break;
    default: fill(0, ' ', VGA_COLOR_GREEN, VGA_COLOR_GREEN);       break;
    }
    ui_reset();
}

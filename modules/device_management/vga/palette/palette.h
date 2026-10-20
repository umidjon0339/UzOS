#ifndef PALETTE_H
#define PALETTE_H

/*
 * palette.h - Colour themes: what the 16 text colours really look like.
 *
 * Text cells only store a colour NUMBER (0-15). The video card turns a
 * number into a real colour with its DAC ("digital-to-analog
 * converter"), a table of 256 red/green/blue values we can rewrite.
 * Change the table, and every character on screen changes colour at
 * once, without touching the text.
 *
 * A theme is one such table of 16 colours, plus the two colours of the
 * wallpaper gradient that the graphics console draws behind the text.
 * Colour 0 ("black") is the background of every theme.
 */

#include <stdint.h>

typedef struct {
    const char *name;
    const char *about;
    uint32_t rgb[16];           // 0x00RRGGBB for colour numbers 0..15
    uint32_t wall_top;          // wallpaper gradient (graphics console)
    uint32_t wall_bottom;
} theme_t;

int palette_theme_count(void);
const theme_t *palette_theme(int index);
const theme_t *palette_current(void);

// Switch to the theme called 'name'. 1 = done, 0 = no such theme,
// -1 = refused: its text would be invisible (contrast_check).
int palette_select(const char *name);

// Install 'theme' if its text is readable. 1 = done, -1 = refused.
int palette_use(const theme_t *theme);

// The real colour of colour number 'color' (0-15) in the current theme.
uint32_t palette_rgb(int color);

// Write the current theme into the VGA DAC (text mode).
void palette_apply_vga(void);

#endif

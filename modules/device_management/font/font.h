#ifndef FONT_H
#define FONT_H

/*
 * font.h - The pictures of the 256 characters.
 *
 * A glyph is 8 pixels wide; each pixel row is one byte (bit 7 = left).
 * Every character exists in two heights:
 *   16 rows  80x25 text mode and the 1024x768 graphics console
 *    8 rows  80x50 text mode (twice the lines, half the height)
 *
 * Two fonts to choose from (`font` command):
 *   FONT_UZOS  the UzOS font (based on Terminus, see font_data.c)
 *   FONT_BIOS  the font the video card had at boot
 * Both get the UzOS icons from glyphs.c on top.
 *
 * The VGA text driver copies the active glyphs into the card; the
 * graphics console draws them pixel by pixel. Both ask font_glyph().
 */

#include <stdint.h>
#include "glyphs.h"

#define FONT_WIDTH 8

typedef enum { FONT_UZOS, FONT_BIOS } font_id_t;

// The tables in font_data.c.
extern const uint8_t font_uzos_8x16[256][16];
extern const uint8_t font_uzos_8x8[256][8];

// Build the UzOS font. Call once at boot.
void font_init(void);

// Give the font module the card's own font (read by the VGA driver at
// boot), so FONT_BIOS can be chosen later.
void font_set_bios(const uint8_t glyphs[256][16]);

// Make 'id' the active font. 0 if it is not available.
int font_select(font_id_t id);
font_id_t font_current(void);
const char *font_name(font_id_t id);

// The active picture of character 'c': 'height' rows (16 or 8).
const uint8_t *font_glyph(uint8_t c, int height);

#endif

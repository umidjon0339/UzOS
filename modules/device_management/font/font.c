/*
 * font.c - Keep the active font, with the UzOS icons drawn in.
 *
 * Choosing a font builds two tables: 'tall' (16 rows per glyph) and
 * 'short' (8 rows). The base glyphs come from the chosen font, then
 * glyphs_draw() paints the icons on top.
 *
 * For the BIOS font there is no 8-row version, so each 8-row glyph is
 * made from the 16-row one by "squeezing": new row r = old rows 2r and
 * 2r+1 ORed together, so thin lines are not lost.
 *
 * Uses: font_data.c (the UzOS tables), glyphs.c (the icons), memcpy.
 */

#include "font.h"
#include "string.h"

static uint8_t bios[256][16];
static int have_bios;

static uint8_t tall[256][16];
static uint8_t short_rows[256][8];
static font_id_t current = FONT_UZOS;

static void squeeze(const uint8_t from[16], uint8_t to[8]) {
    for (int r = 0; r < 8; r++)
        to[r] = from[2 * r] | from[2 * r + 1];
}

// Rebuild 'tall' and 'short_rows' for font 'id'.
static void build(font_id_t id) {
    if (id == FONT_BIOS) {
        memcpy(tall, bios, sizeof(tall));
        for (int c = 0; c < 256; c++)
            squeeze(tall[c], short_rows[c]);
    } else {
        memcpy(tall, font_uzos_8x16, sizeof(tall));
        memcpy(short_rows, font_uzos_8x8, sizeof(short_rows));
    }

    // The icons: drawn at 16 rows, squeezed for the 8-row table.
    for (int i = 0; i < glyphs_count(); i++) {
        uint8_t rows[16];
        uint8_t c = glyphs_get(i, rows);
        memcpy(tall[c], rows, sizeof(rows));
        squeeze(rows, short_rows[c]);
    }
    current = id;
}

void font_init(void) {
    build(FONT_UZOS);
}

void font_set_bios(const uint8_t glyphs[256][16]) {
    memcpy(bios, glyphs, sizeof(bios));
    have_bios = 1;
}

int font_select(font_id_t id) {
    if (id == FONT_BIOS && !have_bios)
        return 0;
    build(id);
    return 1;
}

font_id_t font_current(void) {
    return current;
}

const char *font_name(font_id_t id) {
    return id == FONT_BIOS ? "bios" : "uzos";
}

const uint8_t *font_glyph(uint8_t c, int height) {
    return height == 8 ? short_rows[c] : tall[c];
}

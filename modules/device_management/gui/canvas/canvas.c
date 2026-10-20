/*
 * canvas.c - Rectangles, text and copies on a canvas, always clipped.
 *
 * Uses: font8x8 (next to this file), and the defence canvas_clip for
 *       every rectangle before a single pixel is written.
 */

#include "canvas.h"
#include "font8x8.h"
#include "def_canvas_clip.h"

void canvas_fill(canvas_t *c, uint32_t color) {
    for (int i = 0; i < c->width * c->height; i++)
        c->pixels[i] = color;
}

void canvas_put_pixel(canvas_t *c, int x, int y, uint32_t color) {
    if (x >= 0 && x < c->width && y >= 0 && y < c->height && color != COLOR_TRANSPARENT)
        c->pixels[y * c->width + x] = color;
}

void canvas_fill_rect(canvas_t *c, int x, int y, int w, int h, uint32_t color) {
    // DEFENCE canvas_clip: cut the rectangle down to what is on the canvas.
    if (!def_canvas_clip(c, &x, &y, &w, &h))
        return;

    for (int row = y; row < y + h; row++)
        for (int col = x; col < x + w; col++)
            c->pixels[row * c->width + col] = color;
}

void canvas_draw_char(canvas_t *c, int x, int y, char ch, uint32_t fg, uint32_t bg) {
    uint8_t code = (uint8_t)ch;
    if (code >= 128)
        return;

    const uint8_t *glyph = font8x8_basic[code];
    for (int row = 0; row < FONT_HEIGHT; row++)
        for (int col = 0; col < FONT_WIDTH; col++) {
            int lit = (glyph[row] >> col) & 1;      // bit 0 = leftmost pixel
            canvas_put_pixel(c, x + col, y + row, lit ? fg : bg);
        }
}

void canvas_draw_string(canvas_t *c, int x, int y, const char *s, uint32_t fg, uint32_t bg) {
    int start_x = x;
    for (; *s; s++) {
        if (*s == '\n') {
            x = start_x;
            y += LINE_HEIGHT;
        } else {
            canvas_draw_char(c, x, y, *s, fg, bg);
            x += FONT_WIDTH;
        }
    }
}

void canvas_blit(canvas_t *dst, const canvas_t *src, int x, int y) {
    int w = src->width, h = src->height;
    int clipped_x = x, clipped_y = y;

    // DEFENCE canvas_clip: only the part of 'src' that lands on 'dst'.
    if (!def_canvas_clip(dst, &clipped_x, &clipped_y, &w, &h))
        return;

    int src_x = clipped_x - x;      // how much of src was cut off on the left/top
    int src_y = clipped_y - y;
    for (int row = 0; row < h; row++) {
        uint32_t *to         = &dst->pixels[(clipped_y + row) * dst->width + clipped_x];
        const uint32_t *from = &src->pixels[(src_y + row) * src->width + src_x];
        for (int col = 0; col < w; col++)
            to[col] = from[col];
    }
}

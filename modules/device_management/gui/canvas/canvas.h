#ifndef CANVAS_H
#define CANVAS_H

/*
 * canvas.h - Draw into a block of 32-bit pixels.
 *
 * A canvas is just "pixels + width + height". The screen's back buffer
 * is a canvas, and so is every window. All drawing goes through here,
 * and every shape is clipped to the canvas first (canvas_clip), so no
 * coordinate can ever write outside the pixel buffer.
 *
 * Pixel (x, y) is pixels[y * width + x]; colours are 0x00RRGGBB.
 */

#include <stdint.h>

#define COLOR_BLACK        0x000000
#define COLOR_WHITE        0xFFFFFF
#define COLOR_GREY         0x888888
#define COLOR_LIGHT_GREY   0xDDDDDD
#define COLOR_RED          0xFF0000
#define COLOR_GREEN        0x00CC44
#define COLOR_BLUE         0x2060D0
#define COLOR_DESKTOP      0x2A3441
#define COLOR_TRANSPARENT  0x01000000  // not a real colour: "leave the pixel alone"

#define LINE_HEIGHT        10          // 8 pixel font + 2 pixels gap

typedef struct {
    uint32_t *pixels;
    int width;
    int height;
} canvas_t;

void canvas_fill(canvas_t *c, uint32_t color);
void canvas_put_pixel(canvas_t *c, int x, int y, uint32_t color);
void canvas_fill_rect(canvas_t *c, int x, int y, int w, int h, uint32_t color);

// Text in the 8x8 font. 'bg' may be COLOR_TRANSPARENT. '\n' starts a new line.
void canvas_draw_char(canvas_t *c, int x, int y, char ch, uint32_t fg, uint32_t bg);
void canvas_draw_string(canvas_t *c, int x, int y, const char *s, uint32_t fg, uint32_t bg);

// Copy all of 'src' onto 'dst' with its top-left corner at (x, y).
void canvas_blit(canvas_t *dst, const canvas_t *src, int x, int y);

#endif

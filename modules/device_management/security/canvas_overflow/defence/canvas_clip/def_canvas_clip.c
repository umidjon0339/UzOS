/*
 * def_canvas_clip.c - DEFENCE canvas_clip: drawing never leaves the canvas.
 *
 * Stops:  canvas_overflow (security/canvas_overflow)
 * Called: canvas_fill_rect() and canvas_blit() (gui/canvas), before any
 *         pixel is written.
 * Rule:   cut every rectangle to 0 <= x < width, 0 <= y < height. The
 *         math is done in 64 bits, so x + w cannot overflow.
 * Why:    pixel (x, y) lives at pixels[y * width + x]. With x = -8 that
 *         is 8 pixels BEFORE the row, which for row 0 is before the
 *         whole buffer: the heap block in front gets overwritten. Windows
 *         dragged half off-screen do this all the time, so clipping is
 *         normal and is NOT logged.
 */

#include "def_canvas_clip.h"
#include "registry.h"

static int64_t max64(int64_t a, int64_t b) { return a > b ? a : b; }
static int64_t min64(int64_t a, int64_t b) { return a < b ? a : b; }

// The whole rule, as a pure function on numbers.
static int clip(int canvas_w, int canvas_h, int *x, int *y, int *w, int *h) {
    if (*w <= 0 || *h <= 0)
        return 0;

    int64_t left   = max64(*x, 0);
    int64_t top    = max64(*y, 0);
    int64_t right  = min64((int64_t)*x + *w, canvas_w);   // 64-bit: no overflow
    int64_t bottom = min64((int64_t)*y + *h, canvas_h);

    if (left >= right || top >= bottom)
        return 0;                                  // completely outside

    *x = (int)left;
    *y = (int)top;
    *w = (int)(right - left);
    *h = (int)(bottom - top);
    return 1;
}

int def_canvas_clip(const canvas_t *c, int *x, int *y, int *w, int *h) {
    return clip(c->width, c->height, x, y, w, h);
}

static int selftest(void) {
    int x, y, w, h;

    x = 2; y = 2; w = 4; h = 4;                         // inside: unchanged
    if (!clip(10, 10, &x, &y, &w, &h) || x != 2 || w != 4) return 0;

    x = -8; y = -8; w = 12; h = 12;                     // hangs off the top-left
    if (!clip(10, 10, &x, &y, &w, &h) || x != 0 || y != 0 || w != 4 || h != 4) return 0;

    x = 8; y = 0; w = 100; h = 1;                       // runs off the right
    if (!clip(10, 10, &x, &y, &w, &h) || w != 2) return 0;

    x = 0x7FFFFFF0; y = 0; w = 0x7FFFFFF0; h = 1;       // x + w would overflow
    if (clip(10, 10, &x, &y, &w, &h)) return 0;

    x = 20; y = 20; w = 5; h = 5;                       // completely outside
    if (clip(10, 10, &x, &y, &w, &h)) return 0;
    return 1;
}

static const defence_t defence = {
    "canvas_clip",
    "cut every rectangle to the canvas before drawing",
    "canvas_overflow",
    selftest
};

void def_canvas_clip_register(void) {
    registry_add_defence(&defence);
}

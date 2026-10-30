/*
 * fbcon.c - The graphics console (a display, see display.h), in five sizes:
 *           800x600, 1024x768, 1280x720, 1600x900 and 1920x1080.
 *
 * Drawing one cell = 16 rows of 8 pixels:
 *
 *     for each pixel row r of the glyph:
 *         for each of the 8 bits:  1 -> foreground colour
 *                                  0 -> background colour (or wallpaper)
 *
 * Drawing pixels is slow compared to text mode, so 'shown' remembers
 * what every cell shows right now; put() skips a cell that would not
 * change. When the console scrolls, only cells with new text are drawn.
 *
 * The cursor is a 2-pixel underline drawn into its cell.
 *
 * All sizes share the same code: only open() differs, it tells the
 * card which size to use. The size then lives in 'width' and 'height'.
 *
 * Uses: bga_* (gui/bga), font_glyph (font/), palette_* (vga/palette).
 */

#include "fbcon.h"
#include "bga.h"
#include "font.h"
#include "palette.h"

#define CELL_W      8
#define CELL_H      16
#define MAX_HEIGHT  1080                // the tallest size below

static int width, height;               // the current size in pixels
static uint32_t *fb;                    // the visible pixels
static uint32_t wallpaper[MAX_HEIGHT];  // colour of each pixel row
static uint16_t shown[VGA_MAX_ROWS][VGA_MAX_COLS];  // what each cell shows now
static int cursor_x, cursor_y, cursor_visible;

/* ---------- Wallpaper ---------- */

// Mix a and b: t = 0 gives a, t = 'steps' gives b.
static uint32_t blend(uint32_t a, uint32_t b, int t, int steps) {
    uint32_t out = 0;
    for (int shift = 0; shift <= 16; shift += 8) {
        int ca = (a >> shift) & 0xFF, cb = (b >> shift) & 0xFF;
        out |= (uint32_t)(ca + (cb - ca) * t / steps) << shift;
    }
    return out;
}

// Paint the whole screen with the theme's gradient. Afterwards every
// cell shows "nothing", which is what cell value 0 draws: so shown = 0.
static void paint_wallpaper(void) {
    const theme_t *theme = palette_current();
    for (int y = 0; y < height; y++) {
        wallpaper[y] = blend(theme->wall_top, theme->wall_bottom, y, height - 1);
        for (int x = 0; x < width; x++)
            fb[y * width + x] = wallpaper[y];
    }
    for (int y = 0; y < VGA_MAX_ROWS; y++)
        for (int x = 0; x < VGA_MAX_COLS; x++)
            shown[y][x] = 0;
}

/* ---------- One cell ---------- */

static void draw_cell(int x, int y, uint16_t cell, int with_cursor) {
    uint8_t attr = cell >> 8;
    uint32_t fg = palette_rgb(attr & 0x0F);
    int bg_index = attr >> 4;
    const uint8_t *glyph = font_glyph(cell & 0xFF, CELL_H);
    uint32_t *pixel = fb + (y * CELL_H) * width + x * CELL_W;

    for (int r = 0; r < CELL_H; r++) {
        uint32_t bg = bg_index ? palette_rgb(bg_index) : wallpaper[y * CELL_H + r];
        uint8_t bits = glyph[r];
        if (with_cursor && r >= CELL_H - 2)
            bits = 0xFF;                            // the underline cursor
        for (int b = 0; b < CELL_W; b++)
            pixel[b] = (bits & (0x80 >> b)) ? fg : bg;
        pixel += width;
    }
}

static int is_cursor(int x, int y) {
    return cursor_visible && x == cursor_x && y == cursor_y;
}

/* ---------- The display functions ---------- */

static int fb_open(int w, int h) {
    if (!bga_is_available() || !bga_set_video_mode(w, h, 32))
        return 0;
    width = w;
    height = h;
    fb = bga_framebuffer();
    cursor_visible = 0;
    paint_wallpaper();
    return 1;
}

static void fb_close(void) {
    bga_disable();
}

static void put(int x, int y, uint16_t cell) {
    if (shown[y][x] == cell)
        return;                                     // already on screen
    shown[y][x] = cell;
    draw_cell(x, y, cell, is_cursor(x, y));
}

static void cursor(int x, int y, int visible) {
    if (cursor_visible)                             // rub out the old one
        draw_cell(cursor_x, cursor_y, shown[cursor_y][cursor_x], 0);
    cursor_x = x;
    cursor_y = y;
    cursor_visible = visible;
    if (visible)
        draw_cell(x, y, shown[y][x], 1);
}

static void reload(void) {
    cursor_visible = 0;
    paint_wallpaper();                              // the console redraws the text
}

/* ---------- The five sizes ---------- */

// FBCON_MODE(1280, 720) makes the display 'fbcon_1280x720': its own
// open() sets the card to 1280 x 720, everything else is shared.
// 720 / 16 = 45 rows; when the height does not divide by 16 (600, 900,
// 1080) the few pixel rows left at the bottom just show wallpaper.
#define FBCON_MODE(w, h)                                                    \
    static int open_##w##x##h(void) { return fb_open(w, h); }              \
    const display_t fbcon_##w##x##h = {                                     \
        #w "x" #h, "graphics mode, text on a wallpaper",                    \
        (w) / CELL_W, (h) / CELL_H, w, h,                                   \
        open_##w##x##h, fb_close, put, cursor, reload                       \
    };

FBCON_MODE(800, 600)
FBCON_MODE(1024, 768)
FBCON_MODE(1280, 720)
FBCON_MODE(1600, 900)
FBCON_MODE(1920, 1080)

/*
 * fbcon.c - The 1024x768 graphics console (a display, see display.h).
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
 * Uses: bga_* (gui/bga), font_glyph (font/), palette_* (vga/palette).
 */

#include "fbcon.h"
#include "bga.h"
#include "font.h"
#include "palette.h"

#define WIDTH       1024
#define HEIGHT      768
#define CELL_W      8
#define CELL_H      16
#define COLS        (WIDTH / CELL_W)
#define ROWS        (HEIGHT / CELL_H)

static uint32_t *fb;                    // the visible pixels
static uint32_t wallpaper[HEIGHT];      // colour of each pixel row
static uint16_t shown[ROWS][COLS];      // what each cell shows now
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
    for (int y = 0; y < HEIGHT; y++) {
        wallpaper[y] = blend(theme->wall_top, theme->wall_bottom, y, HEIGHT - 1);
        for (int x = 0; x < WIDTH; x++)
            fb[y * WIDTH + x] = wallpaper[y];
    }
    for (int y = 0; y < ROWS; y++)
        for (int x = 0; x < COLS; x++)
            shown[y][x] = 0;
}

/* ---------- One cell ---------- */

static void draw_cell(int x, int y, uint16_t cell, int with_cursor) {
    uint8_t attr = cell >> 8;
    uint32_t fg = palette_rgb(attr & 0x0F);
    int bg_index = attr >> 4;
    const uint8_t *glyph = font_glyph(cell & 0xFF, CELL_H);
    uint32_t *pixel = fb + (y * CELL_H) * WIDTH + x * CELL_W;

    for (int r = 0; r < CELL_H; r++) {
        uint32_t bg = bg_index ? palette_rgb(bg_index) : wallpaper[y * CELL_H + r];
        uint8_t bits = glyph[r];
        if (with_cursor && r >= CELL_H - 2)
            bits = 0xFF;                            // the underline cursor
        for (int b = 0; b < CELL_W; b++)
            pixel[b] = (bits & (0x80 >> b)) ? fg : bg;
        pixel += WIDTH;
    }
}

static int is_cursor(int x, int y) {
    return cursor_visible && x == cursor_x && y == cursor_y;
}

/* ---------- The display functions ---------- */

static int fb_open(void) {
    if (!bga_is_available())
        return 0;
    bga_set_video_mode(WIDTH, HEIGHT, 32);
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

const display_t fbcon_1024x768 = {
    "1024x768", "graphics mode, 128x48 characters on a wallpaper",
    COLS, ROWS, fb_open, fb_close, put, cursor, reload
};

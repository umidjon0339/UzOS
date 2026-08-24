#include "terminal.h"
#include "font8x8_basic.h"
#include "string.h"

/* Font dimensions (font8x8_basic uses 8x8 bitmaps) */
#define FONT_W 8
#define FONT_H 8

/* Framebuffer state */
static uint8_t* fb_addr;
static uint32_t fb_width;
static uint32_t fb_height;
static uint32_t fb_pitch;    /* Bytes per scanline (may include padding) */
static uint32_t fb_bpp;      /* Bytes per pixel (e.g. 4 for 32-bit) */

/* Text grid dimensions (computed from framebuffer size / font size) */
static uint32_t text_cols;
static uint32_t text_rows;

/* Current cursor position in the text grid */
static uint32_t cur_col;
static uint32_t cur_row;

/* Colors (ARGB / BGRX depending on hardware — 0x00RRGGBB for most framebuffers) */
#define COLOR_FG 0x00CCCCCC  /* Light gray text */
#define COLOR_BG 0x00000000  /* Black background */

/* ---- Low-level pixel helpers ---- */

/* Put a single pixel at (x, y) with the given 32-bit color */
static inline void put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x >= fb_width || y >= fb_height) return;
    uint32_t* pixel = (uint32_t*)(fb_addr + y * fb_pitch + x * fb_bpp);
    *pixel = color;
}

/* Draw a single 8x8 character at text grid position (col, row) */
static void draw_char(uint32_t col, uint32_t row, char c) {
    /* Screen pixel origin for this character cell */
    uint32_t px = col * FONT_W;
    uint32_t py = row * FONT_H;

    /* Get the glyph bitmap. Characters outside printable ASCII get a space. */
    int idx = (unsigned char)c;
    if (idx < 0 || idx > 127) idx = ' ';
    const char* glyph = font8x8_basic[idx];

    for (uint32_t gy = 0; gy < FONT_H; gy++) {
        uint8_t bits = (uint8_t)glyph[gy];
        for (uint32_t gx = 0; gx < FONT_W; gx++) {
            uint32_t color = (bits & (1 << gx)) ? COLOR_FG : COLOR_BG;
            put_pixel(px + gx, py + gy, color);
        }
    }
}

/* Clear a single character cell to the background color */
static void clear_cell(uint32_t col, uint32_t row) {
    uint32_t px = col * FONT_W;
    uint32_t py = row * FONT_H;
    for (uint32_t gy = 0; gy < FONT_H; gy++) {
        for (uint32_t gx = 0; gx < FONT_W; gx++) {
            put_pixel(px + gx, py + gy, COLOR_BG);
        }
    }
}

/* ---- Scrolling ---- */

/* Scroll the entire framebuffer up by one text row (FONT_H pixels) */
static void scroll_up(void) {
    /* Number of bytes in one pixel row */
    uint32_t row_pixels = text_rows * FONT_H;
    (void)row_pixels;

    /* Move everything up by FONT_H scanlines */
    uint32_t bytes_to_move = fb_pitch * (fb_height - FONT_H);
    memcpy(fb_addr, fb_addr + fb_pitch * FONT_H, bytes_to_move);

    /* Clear the last text row (bottom FONT_H scanlines) */
    uint32_t clear_start = fb_pitch * (fb_height - FONT_H);
    memset(fb_addr + clear_start, 0, fb_pitch * FONT_H);
}

/* ---- Public API ---- */

void terminal_initialize(uint32_t* fb, uint32_t width, uint32_t height,
                         uint32_t pitch, uint8_t bpp) {
    fb_addr  = (uint8_t*)fb;
    fb_width  = width;
    fb_height = height;
    fb_pitch  = pitch;
    fb_bpp    = bpp / 8; /* Convert bits-per-pixel to bytes-per-pixel */

    text_cols = fb_width  / FONT_W;
    text_rows = fb_height / FONT_H;
    cur_col   = 0;
    cur_row   = 0;

    /* Clear the entire framebuffer to black */
    for (uint32_t y = 0; y < fb_height; y++) {
        memset(fb_addr + y * fb_pitch, 0, fb_width * fb_bpp);
    }
}

void terminal_putchar(char c) {
    if (c == '\n') {
        cur_col = 0;
        cur_row++;
    } else if (c == '\b') {
        if (cur_col > 0) {
            cur_col--;
            clear_cell(cur_col, cur_row);
        }
    } else if (c == '\t') {
        cur_col = (cur_col + 4) & ~3u;
    } else {
        draw_char(cur_col, cur_row, c);
        cur_col++;
    }

    /* Wrap at right edge */
    if (cur_col >= text_cols) {
        cur_col = 0;
        cur_row++;
    }

    /* Scroll if past bottom edge */
    if (cur_row >= text_rows) {
        scroll_up();
        cur_row = text_rows - 1;
    }
}

void terminal_print(const char* str) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        terminal_putchar(str[i]);
    }
}

void terminal_clear(void) {
    for (uint32_t y = 0; y < fb_height; y++) {
        memset(fb_addr + y * fb_pitch, 0, fb_width * fb_bpp);
    }
    cur_col = 0;
    cur_row = 0;
}

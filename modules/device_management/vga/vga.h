#ifndef VGA_H
#define VGA_H

/*
 * vga.h - The text console: where every message of the OS appears.
 *
 * A cell is one character and its colour (low 4 bits foreground, high 4
 * bits background). The console keeps the last 256 lines of cells, so the
 * mouse wheel can scroll back, and shows them on the current display
 * (display.h): 80x25 or 80x50 text mode, or the 1024x768 graphics console.
 *
 *   row 0        status bar (when one is on, see vga_status_*)
 *   rows 1..     the scrolling text
 *
 * A full-screen program (snake, top, startx) can borrow the whole screen
 * with vga_fullscreen(1); text printed meanwhile is kept and appears
 * when it gives the screen back.
 */

#include <stdint.h>
#include "display.h"

enum vga_color {
    VGA_COLOR_BLACK = 0,       VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,       VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,         VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,       VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,   VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10, VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,  VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14, VGA_COLOR_WHITE = 15,
};

// One screen cell: character 'c' in colour 'fg' on 'bg'.
static inline uint16_t vga_cell(char c, enum vga_color fg, enum vga_color bg) {
    return (uint8_t)c | (uint16_t)((fg | bg << 4) << 8);
}

/* ---------- Setup ---------- */

// Boot: build the font, open the 80x25 text display, clear.
void vga_init(void);

// Empty the screen and the scroll-back history.
void vga_clear(void);

/* ---------- Printing ---------- */

void vga_set_color(enum vga_color fg, enum vga_color bg);
uint8_t vga_get_color(void);            // the current colour byte
void vga_set_color_byte(uint8_t color); // put back a vga_get_color() value

void vga_put_char(char c);              // handles '\n', '\r', '\t', '\b'
void vga_print_str(const char *str);
void vga_print_int(int num);
void vga_print_hex(uint32_t val);       // "0x0000ABCD"

int vga_column(void);                   // where the cursor is in its line

/* ---------- The display ---------- */

// Switch to another display. 1 = done; 0 = it cannot open (the old one stays).
int vga_set_display(const display_t *display);
const display_t *vga_display(void);

int vga_cols(void);                     // size of the whole screen
int vga_rows(void);

// The font or the theme changed: show everything again with it.
void vga_reload(void);

// Graphics (startx) used the card: set the display up again and redraw.
void vga_restore_display(void);

/* ---------- Scroll-back ---------- */

// Scroll the view back (lines > 0) or forward (lines < 0) in history.
void vga_scroll_view(int lines);

// Draw the current screen again from history.
void vga_redraw(void);

/* ---------- Status bar (row 0) ---------- */

// Keep row 0 for a status bar (on = 1) or give it back to the text.
void vga_status_enable(int on);
int vga_status_enabled(void);

// Show 'cells' (vga_cols() of them) in the status row.
void vga_status_set(const uint16_t *cells);

/* ---------- Full-screen programs ---------- */

// on = 1: the program owns the screen (it is cleared, the cursor hidden).
// on = 0: the console draws its text and status bar again.
void vga_fullscreen(int on);

// Draw straight onto the screen at column x, row y (0,0 = top left),
// not into history. For full-screen programs.
void vga_put_at(int x, int y, char c, enum vga_color fg, enum vga_color bg);
void vga_write_at(int x, int y, const char *text, enum vga_color fg, enum vga_color bg);

#endif

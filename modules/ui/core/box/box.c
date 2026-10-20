/*
 * box.c - Draw frames, cells and bars.
 *
 * A frame remembers where its left edge is ('left', the cursor column at
 * box_begin) and how wide it is, so box_row_end() knows how many spaces
 * to add before the right edge.
 *
 * Uses: vga_* (device_management), ui_color / ui_reset (core/style),
 *       def_safe_text (defence safe_text), format (lib/format).
 */

#include "box.h"
#include "ui.h"
#include "format.h"
#include "string.h"
#include "def_safe_text.h"

// Code page 437 line pieces.
#define H_LINE    "\xC4"    // ─
#define V_LINE    "\xB3"    // │
#define TOP_LEFT  "\xDA"    // ┌
#define TOP_RIGHT "\xBF"    // ┐
#define BOT_LEFT  "\xC0"    // └
#define BOT_RIGHT "\xD9"    // ┘
#define T_LEFT    "\xC3"    // ├
#define T_RIGHT   "\xB4"    // ┤
#define FULL      "\xDB"    // █
#define LIGHT     "\xB0"    // ░

#define MAX_CELL  128

static int left, width;

static void line(const char *start, const char *end) {
    ui_color(UI_DIM);
    vga_print_str(start);
    while (vga_column() < left + width - 1)
        vga_print_str(H_LINE);
    vga_print_str(end);
    ui_end();
}

void box_begin(const char *title, int w) {
    left = vga_column();
    width = w;

    ui_color(UI_DIM);
    vga_print_str(TOP_LEFT H_LINE " ");
    ui_color(UI_TITLE);
    vga_print_str(title);
    ui_color(UI_DIM);
    vga_print_str(" ");
    while (vga_column() < left + width - 1)
        vga_print_str(H_LINE);
    vga_print_str(TOP_RIGHT);
    ui_end();
}

void box_row(void) {
    for (int i = 0; i < left; i++)
        vga_put_char(' ');
    ui_color(UI_DIM);
    vga_print_str(V_LINE " ");
    ui_reset();
}

void box_row_end(void) {
    while (vga_column() < left + width - 1 && vga_column() > 0)
        vga_put_char(' ');
    ui_color(UI_DIM);
    vga_print_str(V_LINE);
    ui_end();
}

void box_divider(void) {
    for (int i = 0; i < left; i++)
        vga_put_char(' ');
    line(T_LEFT, T_RIGHT);
}

void box_end(void) {
    for (int i = 0; i < left; i++)
        vga_put_char(' ');
    line(BOT_LEFT, BOT_RIGHT);
}

void ui_cell(const char *text, int w) {
    char clean[MAX_CELL + 1];
    if (w > MAX_CELL)
        w = MAX_CELL;
    def_safe_text(text, clean, w);
    vga_print_str(clean);
    for (int n = strlen(clean); n < w; n++)
        vga_put_char(' ');
}

void ui_cell_int(int value, int w) {
    char digits[12];
    int n = format(digits, sizeof(digits), "%d", value);
    for (int i = n; i < w; i++)
        vga_put_char(' ');
    vga_print_str(digits);
}

void ui_bar(uint32_t part, uint32_t whole, int w) {
    uint32_t percent = whole ? (uint64_t)part * 100 / whole : 0;
    int filled = whole ? (int)((uint64_t)part * w / whole) : 0;
    if (part && !filled)
        filled = 1;                     // a little use still shows

    ui_color(percent > 85 ? UI_BAD : percent > 60 ? UI_WARNING : UI_GOOD);
    for (int i = 0; i < filled; i++)
        vga_print_str(FULL);
    ui_color(UI_DIM);
    for (int i = filled; i < w; i++)
        vga_print_str(LIGHT);
    ui_reset();
}

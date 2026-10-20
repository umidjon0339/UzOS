/*
 * vga.c - The console: text with colours, a cursor and scroll-back.
 *
 * All text goes into 'history' (a ring of the last HISTORY_LINES lines).
 * The screen shows the newest 'text_rows' of them, or older ones while
 * the user scrolls back with the mouse wheel. Row 0 of the screen may
 * hold the status bar instead ('top' = 1).
 *
 *   screen row   0         status bar           (if top = 1)
 *                top ...   history lines, the cursor's line at the bottom
 *
 * The console never touches hardware itself: it calls the current
 * display (display.h) to put a cell or move the cursor.
 *
 * Printing turns interrupts off: two processes printing at the same time
 * must not interleave halfway through a cursor update.
 *
 * Uses: the displays (vga/text, gui/fbcon), font_init (font/),
 *       cpu_irq_save (arch/x86/cpu).
 */

#include "vga.h"
#include "vga_text.h"
#include "font.h"
#include "cpu.h"

#define HISTORY_LINES  256
#define TAB_WIDTH      8

static const display_t *display;
static int cols, rows;              // size of the display
static int top;                     // 1 when row 0 is the status bar
static int text_rows;               // rows - top: lines of text on screen
static int fullscreen;              // a program owns the screen

static uint16_t history[HISTORY_LINES][VGA_MAX_COLS];
static int first_line;              // history index of the oldest line
static int line_count;              // lines in use (at least text_rows)
static int scroll_back;             // 0 = showing the newest lines
static int row, col;                // cursor, within the newest text_rows lines
static uint8_t color = VGA_COLOR_LIGHT_GREY;
static uint16_t status[VGA_MAX_COLS];

static uint16_t cell(char c, uint8_t attr) {
    return (uint8_t)c | (attr << 8);
}

// History index of text row 'r' when not scrolled back.
static int line_of_row(int r) {
    return (first_line + line_count - text_rows + r) % HISTORY_LINES;
}

/* ---------- Drawing through the display ---------- */

static void move_cursor(void) {
    if (!fullscreen)
        display->cursor(col, row + top, !scroll_back);  // hidden while scrolled back
}

static void draw_status(void) {
    for (int x = 0; x < cols; x++)
        display->put(x, 0, status[x]);
}

static void render(void) {
    if (fullscreen)
        return;
    int first = line_count - text_rows - scroll_back;     // first visible line
    for (int y = 0; y < text_rows; y++) {
        int line = (first_line + first + y) % HISTORY_LINES;
        for (int x = 0; x < cols; x++)
            display->put(x, y + top, history[line][x]);
    }
    if (top)
        draw_status();
    move_cursor();
}

/* ---------- Writing text ---------- */

static void clear_line(int line) {
    for (int x = 0; x < VGA_MAX_COLS; x++)
        history[line][x] = cell(' ', color);
}

// Write one cell at the cursor, in history and (if visible) on screen.
static void store(char c) {
    history[line_of_row(row)][col] = cell(c, color);
    if (!scroll_back && !fullscreen)
        display->put(col, row + top, cell(c, color));
}

// The cursor went below the last row: start a new line in history.
static void new_line(void) {
    if (line_count == HISTORY_LINES)
        first_line = (first_line + 1) % HISTORY_LINES;   // forget the oldest
    else
        line_count++;

    clear_line(line_of_row(text_rows - 1));
    row = text_rows - 1;

    // Keep showing the same old text while the user is scrolled back.
    if (scroll_back && scroll_back < line_count - text_rows)
        scroll_back++;
    render();
}

static void put_char(char c) {
    switch (c) {
    case '\n':
        col = 0;
        if (++row == text_rows)
            new_line();
        break;
    case '\r':
        col = 0;
        break;
    case '\t':
        do { put_char(' '); } while (col % TAB_WIDTH);
        return;
    case '\b':
        if (col > 0) {
            col--;
        } else if (row > 0) {
            row--;
            col = cols - 1;
        }
        store(' ');
        break;
    default:
        store(c);
        if (++col == cols) {
            col = 0;
            if (++row == text_rows)
                new_line();
        }
    }
    move_cursor();
}

/* ---------- Changing the screen size ---------- */

// The display or the status row changed: fit the history to the new
// number of text rows, keeping the cursor on the line it was on.
static void relayout(int new_cols, int new_rows, int new_top) {
    int cursor_line = line_count - text_rows + row;    // counted from first_line

    cols = new_cols;
    rows = new_rows;
    top = new_top;
    text_rows = rows - top;

    // Too few lines to fill the screen: add empty ones at the bottom.
    while (line_count < text_rows) {
        clear_line((first_line + line_count) % HISTORY_LINES);
        line_count++;
    }

    row = cursor_line - (line_count - text_rows);
    if (row < 0) {                  // lines below the cursor pushed it off: drop them
        line_count = cursor_line + 1 > text_rows ? cursor_line + 1 : text_rows;
        row = cursor_line - (line_count - text_rows);
    }
    if (col >= cols)
        col = cols - 1;
    scroll_back = 0;
}

/* ---------- Public: setup ---------- */

void vga_init(void) {
    font_init();
    display = &vga_text_80x25;
    display->open();
    cols = display->cols;
    rows = text_rows = display->rows;
    vga_clear();
}

void vga_clear(void) {
    uint32_t irq = cpu_irq_save();
    first_line = 0;
    line_count = text_rows;
    scroll_back = 0;
    row = col = 0;
    for (int line = 0; line < HISTORY_LINES; line++)
        clear_line(line);
    render();
    cpu_irq_restore(irq);
}

/* ---------- Public: printing ---------- */

void vga_set_color(enum vga_color fg, enum vga_color bg) {
    color = fg | (bg << 4);
}

uint8_t vga_get_color(void) {
    return color;
}

void vga_set_color_byte(uint8_t c) {
    color = c;
}

void vga_put_char(char c) {
    uint32_t irq = cpu_irq_save();
    put_char(c);
    cpu_irq_restore(irq);
}

void vga_print_str(const char *str) {
    uint32_t irq = cpu_irq_save();      // the whole string in one piece
    while (*str)
        put_char(*str++);
    cpu_irq_restore(irq);
}

void vga_print_int(int num) {
    char digits[12];
    int n = 0;
    // Work in unsigned: -INT_MIN does not fit in an int.
    uint32_t value = num < 0 ? -(uint32_t)num : (uint32_t)num;

    do {
        digits[n++] = '0' + value % 10;
        value /= 10;
    } while (value);
    if (num < 0)
        digits[n++] = '-';

    uint32_t irq = cpu_irq_save();
    while (n)
        put_char(digits[--n]);
    cpu_irq_restore(irq);
}

void vga_print_hex(uint32_t val) {
    static const char hex[] = "0123456789ABCDEF";
    uint32_t irq = cpu_irq_save();
    put_char('0');
    put_char('x');
    for (int shift = 28; shift >= 0; shift -= 4)
        put_char(hex[(val >> shift) & 0xF]);
    cpu_irq_restore(irq);
}

int vga_column(void) {
    return col;
}

/* ---------- Public: the display ---------- */

int vga_set_display(const display_t *next) {
    uint32_t irq = cpu_irq_save();
    const display_t *old = display;

    old->close();
    if (!next->open()) {
        old->open();                    // could not switch: keep the old one
        render();
        cpu_irq_restore(irq);
        return 0;
    }
    display = next;
    relayout(next->cols, next->rows, top);
    render();
    cpu_irq_restore(irq);
    return 1;
}

const display_t *vga_display(void) {
    return display;
}

int vga_cols(void) {
    return cols;
}

int vga_rows(void) {
    return rows;
}

void vga_reload(void) {
    uint32_t irq = cpu_irq_save();
    display->reload();
    render();
    cpu_irq_restore(irq);
}

void vga_restore_display(void) {
    uint32_t irq = cpu_irq_save();
    display->open();
    render();
    cpu_irq_restore(irq);
}

/* ---------- Public: scroll-back ---------- */

void vga_scroll_view(int lines) {
    uint32_t irq = cpu_irq_save();
    int max_back = line_count - text_rows;

    scroll_back += lines;
    if (scroll_back < 0) scroll_back = 0;
    if (scroll_back > max_back) scroll_back = max_back;

    render();
    cpu_irq_restore(irq);
}

void vga_redraw(void) {
    uint32_t irq = cpu_irq_save();
    render();
    cpu_irq_restore(irq);
}

/* ---------- Public: status bar ---------- */

void vga_status_enable(int on) {
    uint32_t irq = cpu_irq_save();
    on = on ? 1 : 0;
    if (on != top) {
        for (int x = 0; x < VGA_MAX_COLS; x++)
            status[x] = cell(' ', color);
        relayout(cols, rows, on);
        render();
    }
    cpu_irq_restore(irq);
}

int vga_status_enabled(void) {
    return top;
}

void vga_status_set(const uint16_t *cells) {
    uint32_t irq = cpu_irq_save();
    for (int x = 0; x < cols; x++)
        status[x] = cells[x];
    if (top && !fullscreen)
        draw_status();
    cpu_irq_restore(irq);
}

/* ---------- Public: full-screen programs ---------- */

void vga_fullscreen(int on) {
    uint32_t irq = cpu_irq_save();
    if (on) {
        display->cursor(0, 0, 0);
        fullscreen = 1;
        for (int y = 0; y < rows; y++)          // a clean screen to start from
            for (int x = 0; x < cols; x++)
                display->put(x, y, cell(' ', VGA_COLOR_LIGHT_GREY));
    } else {
        fullscreen = 0;
        render();
    }
    cpu_irq_restore(irq);
}

void vga_put_at(int x, int y, char c, enum vga_color fg, enum vga_color bg) {
    if (x < 0 || x >= cols || y < 0 || y >= rows)
        return;
    uint32_t irq = cpu_irq_save();
    display->put(x, y, cell(c, fg | (bg << 4)));
    cpu_irq_restore(irq);
}

void vga_write_at(int x, int y, const char *text, enum vga_color fg, enum vga_color bg) {
    for (; *text; text++, x++)
        vga_put_at(x, y, *text, fg, bg);
}

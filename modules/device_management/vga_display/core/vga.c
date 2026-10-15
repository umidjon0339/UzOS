#include "vga.h"
#include "io.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define HISTORY_LINES 256

static uint16_t* const VGA_MEMORY = (uint16_t*) 0xB8000;

static size_t terminal_row;
static size_t terminal_col;
static uint8_t terminal_color;
static uint16_t terminal_history[HISTORY_LINES][VGA_WIDTH];
static size_t history_start;
static size_t history_count;
static size_t scroll_offset;

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
    return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t) uc | (uint16_t) color << 8;
}

static void update_cursor(size_t x, size_t y) {
    if (scroll_offset) {
        outb(0x3D4, 0x0A);
        outb(0x3D5, 0x20);
        return;
    }

    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x0E);
    uint16_t pos = y * VGA_WIDTH + x;
    
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t) (pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}

static void render_view(void) {
    size_t available = history_count - VGA_HEIGHT;
    size_t first = available - scroll_offset;

    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        size_t line = (history_start + first + y) % HISTORY_LINES;
        for (size_t x = 0; x < VGA_WIDTH; x++)
            VGA_MEMORY[y * VGA_WIDTH + x] = terminal_history[line][x];
    }

    if (!scroll_offset) update_cursor(terminal_col, terminal_row);
    else update_cursor(0, 0);
}

static void clear_history_line(size_t line) {
    for (size_t x = 0; x < VGA_WIDTH; x++)
        terminal_history[line][x] = vga_entry(' ', terminal_color);
}

void vga_init(void) {
    terminal_row = 0;
    terminal_col = 0;
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    history_start = 0;
    history_count = VGA_HEIGHT;
    scroll_offset = 0;

    for (size_t y = 0; y < HISTORY_LINES; y++)
        clear_history_line(y);
    render_view();
}

void vga_set_color(enum vga_color fg, enum vga_color bg) {
    terminal_color = vga_entry_color(fg, bg);
}

static void vga_scroll(void) {
    if (history_count == HISTORY_LINES) {
        history_start = (history_start + 1) % HISTORY_LINES;
    } else {
        history_count++;
    }

    size_t line = (history_start + history_count - 1) % HISTORY_LINES;
    clear_history_line(line);
    terminal_row = VGA_HEIGHT - 1;

    size_t max_offset = history_count - VGA_HEIGHT;
    if (scroll_offset && scroll_offset < max_offset) scroll_offset++;
    else if (scroll_offset > max_offset) scroll_offset = max_offset;
    render_view();
}

void vga_scroll_view(int lines) {
    size_t max_offset = history_count - VGA_HEIGHT;

    if (lines > 0) {
        size_t amount = (size_t)lines;
        scroll_offset = amount > max_offset - scroll_offset
            ? max_offset
            : scroll_offset + amount;
    } else if (lines < 0) {
        size_t amount = (size_t)(-lines);
        scroll_offset = amount > scroll_offset ? 0 : scroll_offset - amount;
    }

    render_view();
}

void vga_put_char(char c) {
    if (c == '\n') {
        terminal_col = 0;
        if (++terminal_row == VGA_HEIGHT) {
            vga_scroll();
        }
        update_cursor(terminal_col, terminal_row);
        return;
    }
    
    if (c == '\t') {
        int space_count = 8 - (terminal_col % 8);
        for (int i = 0; i < space_count; i++) {
            vga_put_char(' ');
        }
        return;
    }

    if (c == '\b') {
        if (terminal_col > 0) {
            terminal_col--;
        } else if (terminal_row > 0) {
            terminal_row--;
            terminal_col = VGA_WIDTH - 1;
        }
        const size_t index = terminal_row * VGA_WIDTH + terminal_col;
        size_t line = (history_start + history_count - VGA_HEIGHT + terminal_row)
                    % HISTORY_LINES;
        terminal_history[line][terminal_col] = vga_entry(' ', terminal_color);
        if (!scroll_offset) VGA_MEMORY[index] = terminal_history[line][terminal_col];
        update_cursor(terminal_col, terminal_row);
        return;
    }

    const size_t index = terminal_row * VGA_WIDTH + terminal_col;
    size_t line = (history_start + history_count - VGA_HEIGHT + terminal_row)
                % HISTORY_LINES;
    terminal_history[line][terminal_col] = vga_entry((unsigned char)c, terminal_color);
    if (!scroll_offset) VGA_MEMORY[index] = terminal_history[line][terminal_col];
    
    if (++terminal_col == VGA_WIDTH) {
        terminal_col = 0;
        if (++terminal_row == VGA_HEIGHT) {
            vga_scroll();
        }
    }
    update_cursor(terminal_col, terminal_row);
}

void vga_print_str(const char* str) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        vga_put_char(str[i]);
    }
}

void vga_print_int(int num) {
    if (num == 0) {
        vga_put_char('0');
        return;
    }
    
    if (num < 0) {
        vga_put_char('-');
        num = -num;
    }
    
    char buffer[16];
    int i = 0;
    while (num > 0) {
        buffer[i++] = (num % 10) + '0';
        num /= 10;
    }
    
    while (i > 0) {
        vga_put_char(buffer[--i]);
    }
}

void vga_print_hex(uint32_t val) {
    vga_print_str("0x");
    char hex_chars[] = "0123456789ABCDEF";
    for (int i = 28; i >= 0; i -= 4) {
        vga_put_char(hex_chars[(val >> i) & 0x0F]);
    }
}

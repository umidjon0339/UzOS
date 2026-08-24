#include "terminal.h"

// The physical address of the VGA text buffer in memory.
// Because we identity mapped the first 2MB of memory in our page tables,
// we can safely access this physical address directly in our C code.
static uint16_t* const VGA_BUFFER = (uint16_t*)0xB8000;

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;

static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;

// Creates a VGA character. Each character on the screen takes 2 bytes:
// Byte 0: The ASCII character
// Byte 1: The color (background in upper 4 bits, foreground in lower 4 bits)
static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t) uc | (uint16_t) color << 8;
}

void terminal_initialize(void) {
    terminal_row = 0;
    terminal_column = 0;
    // Color 0x0F means White (F) foreground on Black (0) background
    terminal_color = 0x0F; 

    // Clear the screen by filling it with empty spaces
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            VGA_BUFFER[index] = vga_entry(' ', terminal_color);
        }
    }
}

static void terminal_putc(char c) {
    if (c == '\n') {
        terminal_column = 0;
        terminal_row++;
    } else {
        const size_t index = terminal_row * VGA_WIDTH + terminal_column;
        VGA_BUFFER[index] = vga_entry(c, terminal_color);
        terminal_column++;
    }

    // Wrap around to the next line if we hit the right edge
    if (terminal_column >= VGA_WIDTH) {
        terminal_column = 0;
        terminal_row++;
    }

    // Simple scroll (clear screen and reset) if we hit the bottom
    if (terminal_row >= VGA_HEIGHT) {
        terminal_initialize();
    }
}

void terminal_print(const char* str) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        terminal_putc(str[i]);
    }
}

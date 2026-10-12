#include "../fs_commands_internal.h"
#include "fs_syscalls.h"
#include "vga.h"
#include "string.h"

// Helper to convert byte to hex string (2 chars)
static void byte_to_hex(unsigned char b, char *out) {
    const char *hex = "0123456789ABCDEF";
    out[0] = hex[(b >> 4) & 0x0F];
    out[1] = hex[b & 0x0F];
    out[2] = '\0';
}

// Helper to convert uint32 to hex string (8 chars)
static void uint32_to_hex(uint32_t val, char *out) {
    const char *hex = "0123456789ABCDEF";
    for (int i = 7; i >= 0; i--) {
        out[i] = hex[val & 0x0F];
        val >>= 4;
    }
    out[8] = '\0';
}

// Helper to print uint32
static void print_num(uint32_t num) {
    if (num == 0) {
        vga_print_str("0");
        return;
    }
    char buf[16];
    int idx = 0;
    while (num > 0) {
        buf[idx++] = '0' + (num % 10);
        num /= 10;
    }
    for (int i = idx - 1; i >= 0; i--) {
        vga_put_char(buf[i]);
    }
}

// Helper for substring matching
static int str_contains(const char *haystack, const char *needle) {
    if (!*needle) return 1;
    for (; *haystack; haystack++) {
        const char *h = haystack;
        const char *n = needle;
        while (*h && *n && *h == *n) {
            h++;
            n++;
        }
        if (!*n) return 1;
    }
    return 0;
}

// cmd_wc: word, line, byte count
void cmd_wc(const char *args) {
    if (args[0] == '\0') {
        vga_print_str("Usage: wc <file>\n");
        return;
    }

    int fd = sys_open(args);
    if (fd == -1) {
        vga_print_str("Failed to open file.\n");
        return;
    }

    char buffer[512];
    int bytes_read;
    uint32_t lines = 0, words = 0, bytes = 0;
    int in_word = 0;

    while ((bytes_read = sys_read(fd, buffer, sizeof(buffer))) > 0) {
        bytes += bytes_read;
        for (int i = 0; i < bytes_read; i++) {
            char c = buffer[i];
            if (c == '\n') lines++;
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                in_word = 0;
            } else if (!in_word) {
                in_word = 1;
                words++;
            }
        }
    }
    sys_close(fd);

    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print_str("  Lines: ");
    print_num(lines);
    vga_print_str(" | Words: ");
    print_num(words);
    vga_print_str(" | Bytes: ");
    print_num(bytes);
    vga_print_str(" -> ");
    vga_print_str(args);
    vga_print_str("\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}

// cmd_grep: pattern search in a file
void cmd_grep(const char *args) {
    if (args[0] == '\0') {
        vga_print_str("Usage: grep <pattern> <file>\n");
        return;
    }

    // Extract pattern (first arg) and filename (second arg)
    char pattern[64];
    char filename[64];
    int i = 0;

    while (args[i] && args[i] != ' ' && i < 63) {
        pattern[i] = args[i];
        i++;
    }
    pattern[i] = '\0';

    if (args[i] != ' ') {
        vga_print_str("Usage: grep <pattern> <file>\n");
        return;
    }

    i++; // Skip space
    int j = 0;
    while (args[i] && j < 63) {
        filename[j++] = args[i++];
    }
    filename[j] = '\0';

    int fd = sys_open(filename);
    if (fd == -1) {
        vga_print_str("Failed to open file.\n");
        return;
    }

    char buffer[1024];
    int bytes_read = sys_read(fd, buffer, sizeof(buffer) - 1);
    sys_close(fd);

    if (bytes_read <= 0) {
        vga_print_str("File empty or error reading.\n");
        return;
    }
    buffer[bytes_read] = '\0';

    // Scan line by line for pattern match
    char line[256];
    int line_idx = 0;
    int line_num = 1;
    int matches = 0;

    for (int k = 0; k <= bytes_read; k++) {
        char c = buffer[k];
        if (c == '\n' || c == '\0') {
            line[line_idx] = '\0';

            // Check if pattern is substring of line
            if (str_contains(line, pattern)) {
                matches++;
                vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
                print_num(line_num);
                vga_print_str(": ");
                vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
                vga_print_str(line);
                vga_print_str("\n");
            }

            line_idx = 0;
            line_num++;
            if (c == '\0') break;
        } else if (line_idx < 255) {
            line[line_idx++] = c;
        }
    }

    if (matches == 0) {
        vga_print_str("No matches found for pattern.\n");
    }
}

// cmd_hexdump: display hex and ASCII view
void cmd_hexdump(const char *args) {
    if (args[0] == '\0') {
        vga_print_str("Usage: hexdump <file>\n");
        return;
    }

    int fd = sys_open(args);
    if (fd == -1) {
        vga_print_str("Failed to open file.\n");
        return;
    }

    unsigned char buffer[256];
    int bytes_read = sys_read(fd, (char*)buffer, sizeof(buffer));
    sys_close(fd);

    if (bytes_read <= 0) {
        vga_print_str("File empty or error reading.\n");
        return;
    }

    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print_str("Offset    00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F  ASCII\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    for (int offset = 0; offset < bytes_read; offset += 16) {
        char hex_addr[9];
        uint32_to_hex((uint32_t)offset, hex_addr);
        vga_set_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
        vga_print_str(hex_addr);
        vga_print_str("  ");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

        // Print hex bytes
        for (int b = 0; b < 16; b++) {
            if (offset + b < bytes_read) {
                char h[3];
                byte_to_hex(buffer[offset + b], h);
                vga_print_str(h);
                vga_print_str(" ");
            } else {
                vga_print_str("   ");
            }
        }

        vga_print_str(" ");

        // Print ASCII characters
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        for (int b = 0; b < 16; b++) {
            if (offset + b < bytes_read) {
                unsigned char c = buffer[offset + b];
                if (c >= 32 && c <= 126) {
                    vga_put_char((char)c);
                } else {
                    vga_put_char('.');
                }
            }
        }
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        vga_print_str("\n");
    }
}

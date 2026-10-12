#include "shell.h"
#include "vga.h"
#include "string.h"

// Map color names to VGA color constants
static int parse_color(const char *name) {
    if (strcmp(name, "black")   == 0) return VGA_COLOR_BLACK;
    if (strcmp(name, "blue")    == 0) return VGA_COLOR_BLUE;
    if (strcmp(name, "green")   == 0) return VGA_COLOR_GREEN;
    if (strcmp(name, "cyan")    == 0) return VGA_COLOR_CYAN;
    if (strcmp(name, "red")     == 0) return VGA_COLOR_RED;
    if (strcmp(name, "magenta") == 0) return VGA_COLOR_MAGENTA;
    if (strcmp(name, "brown")   == 0) return VGA_COLOR_BROWN;
    if (strcmp(name, "grey")    == 0) return VGA_COLOR_LIGHT_GREY;
    if (strcmp(name, "gray")    == 0) return VGA_COLOR_LIGHT_GREY;
    if (strcmp(name, "darkgrey")  == 0) return VGA_COLOR_DARK_GREY;
    if (strcmp(name, "darkgray")  == 0) return VGA_COLOR_DARK_GREY;
    if (strcmp(name, "lightblue") == 0) return VGA_COLOR_LIGHT_BLUE;
    if (strcmp(name, "lightgreen")== 0) return VGA_COLOR_LIGHT_GREEN;
    if (strcmp(name, "lightcyan") == 0) return VGA_COLOR_LIGHT_CYAN;
    if (strcmp(name, "lightred")  == 0) return VGA_COLOR_LIGHT_RED;
    if (strcmp(name, "pink")     == 0) return VGA_COLOR_LIGHT_MAGENTA;
    if (strcmp(name, "yellow")   == 0) return VGA_COLOR_LIGHT_BROWN;
    if (strcmp(name, "white")    == 0) return VGA_COLOR_WHITE;
    return -1;
}

void cmd_color(const char *args) {
    if (args[0] == '\0') {
        vga_print_str("\nUsage: color <fg> [bg]\n");
        vga_print_str("Colors: black, blue, green, cyan, red, magenta, brown,\n");
        vga_print_str("        grey, darkgrey, lightblue, lightgreen, lightcyan,\n");
        vga_print_str("        lightred, pink, yellow, white\n");
        vga_print_str("Example: color lightgreen black\n");
        return;
    }

    // Parse foreground color (first word)
    char fg_name[32];
    char bg_name[32];
    int i = 0;
    while (args[i] && args[i] != ' ' && i < 31) {
        fg_name[i] = args[i];
        i++;
    }
    fg_name[i] = '\0';

    // Parse optional background color (second word)
    int has_bg = 0;
    if (args[i] == ' ') {
        i++; // skip space
        int j = 0;
        while (args[i] && args[i] != ' ' && j < 31) {
            bg_name[j++] = args[i++];
        }
        bg_name[j] = '\0';
        has_bg = 1;
    }

    int fg = parse_color(fg_name);
    if (fg == -1) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_print_str("\nUnknown color: ");
        vga_print_str(fg_name);
        vga_print_str("\n");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return;
    }

    int bg = VGA_COLOR_BLACK; // default background
    if (has_bg) {
        bg = parse_color(bg_name);
        if (bg == -1) {
            vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
            vga_print_str("\nUnknown color: ");
            vga_print_str(bg_name);
            vga_print_str("\n");
            vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
            return;
        }
    }

    vga_set_color((enum vga_color)fg, (enum vga_color)bg);
    vga_print_str("\nColor changed!\n");
}

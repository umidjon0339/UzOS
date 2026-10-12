#ifndef BOOT_LOGO_H
#define BOOT_LOGO_H

#include "vga.h"

static inline void print_boot_logo(void) {
    // Set a striking color for the logo
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print_str("\n");
    vga_print_str("    ##   ##  ######   #####    ######  \n");
    vga_print_str("    ##   ##     ##   ##   ##  ##       \n");
    vga_print_str("    ##   ##    ##    ##   ##   #####   \n");
    vga_print_str("    ##   ##   ##     ##   ##       ##  \n");
    vga_print_str("     #####   ######   #####   ######   \n");
    
    // Version info in different color
    vga_set_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK);
    vga_print_str("           Uzbekistan Operating System\n");
    
    vga_set_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK);
    vga_print_str("          ──────────────────────────────\n");
    
    vga_set_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
    vga_print_str("              v1.0  |  i386  |  2026\n");
    
    vga_set_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK);
    vga_print_str("          ──────────────────────────────\n\n");
    
    // Reset color
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
}

#endif

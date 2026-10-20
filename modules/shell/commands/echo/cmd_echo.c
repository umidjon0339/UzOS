/*
 * cmd_echo.c - `echo <text>`: print the text back.
 *
 * Uses: vga_print_str (device_management).
 */

#include "shell_commands.h"
#include "vga.h"

void cmd_echo(const char *args) {
    vga_print_str(args);
    vga_print_str("\n");
}

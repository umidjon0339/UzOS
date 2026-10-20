/*
 * cmd_clear.c - `clear`: empty the screen (and the scroll-back).
 *
 * Uses: vga_clear (device_management). The status bar stays.
 */

#include "shell_commands.h"
#include "vga.h"

void cmd_clear(const char *args) {
    (void)args;
    vga_clear();
}

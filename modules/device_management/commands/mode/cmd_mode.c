/*
 * cmd_mode.c - `mode [name]`: list the displays, or switch to one.
 *
 *   80x25      text mode (the card draws the characters)
 *   80x50      text mode with a half-height font
 *   800x600 ... 1920x1080   the graphics console (we draw every pixel)
 *
 * The text is kept: the console fits its history to the new size.
 * Ctrl+Up / Ctrl+Down step through the same list without typing.
 *
 * Uses: modes_* (vga/modes), vga_set_display (vga/), ui_* (ui module).
 */

#include "device_commands.h"
#include "vga.h"
#include "modes.h"
#include "ui.h"
#include "box.h"

static void list(void) {
    ui_title("Display modes");
    for (int i = 0; i < modes_count(); i++) {
        const display_t *mode = modes_at(i);
        int active = mode == vga_display();
        ui_color(UI_GOOD);
        vga_print_str(active ? GLYPH_CHECK " " : "  ");
        ui_color(active ? UI_TITLE : VGA_COLOR_WHITE);
        ui_cell(mode->name, 10);
        ui_color(UI_DIM);
        ui_printf("%3dx%-3d ", mode->cols, mode->rows);
        vga_print_str(mode->about);
        ui_end();
    }
    ui_hint("mode <name> switches, e.g. mode 1280x720  (or Ctrl+Up / Ctrl+Down)");
}

void cmd_mode(const char *args) {
    if (!args[0]) {
        list();
        return;
    }
    const display_t *mode = modes_find(args);
    if (!mode) {
        ui_error("mode", "no such mode (type `mode` for the list)");
        return;
    }
    if (!vga_set_display(mode)) {
        ui_error("mode", "this display cannot be opened (no BGA video card?)");
        return;
    }
    ui_start(UI_OK, "mode");
    ui_printf("%s: %d columns, %d rows", mode->name, vga_cols(), vga_rows());
    ui_end();
}

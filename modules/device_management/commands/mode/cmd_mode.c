/*
 * cmd_mode.c - `mode [name]`: list the displays, or switch to one.
 *
 *   80x25      text mode (the card draws the characters)
 *   80x50      text mode with a half-height font
 *   1024x768   the graphics console (we draw every pixel)
 *
 * The text is kept: the console fits its history to the new size.
 *
 * Uses: vga_set_display (vga/), the displays (vga/text, gui/fbcon),
 *       ui_* (ui module).
 */

#include "device_commands.h"
#include "vga.h"
#include "vga_text.h"
#include "fbcon.h"
#include "string.h"
#include "ui.h"
#include "box.h"

static const display_t *const displays[] = {
    &vga_text_80x25, &vga_text_80x50, &fbcon_1024x768,
};
#define DISPLAY_COUNT (sizeof(displays) / sizeof(displays[0]))

static void list(void) {
    ui_title("Display modes");
    for (unsigned i = 0; i < DISPLAY_COUNT; i++) {
        int active = displays[i] == vga_display();
        ui_color(UI_GOOD);
        vga_print_str(active ? GLYPH_CHECK " " : "  ");
        ui_color(active ? UI_TITLE : VGA_COLOR_WHITE);
        ui_cell(displays[i]->name, 10);
        ui_color(UI_DIM);
        ui_printf("%3dx%-3d ", displays[i]->cols, displays[i]->rows);
        vga_print_str(displays[i]->about);
        ui_end();
    }
    ui_hint("mode <name> switches, e.g. mode 80x50");
}

void cmd_mode(const char *args) {
    if (!args[0]) {
        list();
        return;
    }
    for (unsigned i = 0; i < DISPLAY_COUNT; i++) {
        if (strcmp(args, displays[i]->name) != 0)
            continue;
        if (!vga_set_display(displays[i])) {
            ui_error("mode", "this display cannot be opened (no BGA video card?)");
            return;
        }
        ui_start(UI_OK, "mode");
        ui_printf("%s: %d columns, %d rows", displays[i]->name, vga_cols(), vga_rows());
        ui_end();
        return;
    }
    ui_error("mode", "no such mode (type `mode` for the list)");
}

/*
 * cmd_theme.c - `theme [name]`: list the colour themes, or switch to one.
 *
 * Switching only rewrites the 16 colours (palette/); the text on screen
 * stays the same and changes colour at once.
 *
 * Uses: palette_* (vga/palette), vga_reload (vga/), ui_* (ui module).
 */

#include "device_commands.h"
#include "palette.h"
#include "ui.h"
#include "box.h"

#define SWATCH '\xDB'           // █

// The screen can only show 16 colours at a time, so only the current
// theme's colours can be shown as swatches.
static void list(void) {
    ui_title("Themes");
    for (int i = 0; i < palette_theme_count(); i++) {
        const theme_t *t = palette_theme(i);
        int active = t->name == palette_current()->name;

        ui_color(UI_GOOD);
        vga_print_str(active ? GLYPH_CHECK " " : "  ");
        ui_color(active ? UI_TITLE : ui_text_fg());
        ui_cell(t->name, 9);
        ui_color(UI_DIM);
        vga_print_str(t->about);
        ui_end();
    }
    ui_color(UI_LABEL);
    vga_print_str("  colours  ");
    for (int c = 0; c < 16; c++) {
        vga_set_color(c, c == 0 ? VGA_COLOR_DARK_GREY : ui_text_bg());   // 0 = the background
        vga_put_char(SWATCH);
        vga_put_char(SWATCH);
    }
    ui_end();
    ui_hint("theme <name> switches; everything on screen changes colour at once.");
}

void cmd_theme(const char *args) {
    if (!args[0]) {
        list();
        return;
    }
    int result = palette_select(args);
    if (result == 0) {
        ui_error("theme", "no such theme (type `theme` for the list)");
        return;
    }
    if (result < 0) {
        ui_error("theme", "refused: its text would be unreadable (contrast_check)");
        return;
    }
    vga_reload();
    ui_start(UI_OK, "theme");
    vga_print_str(palette_current()->name);
    ui_end();
}

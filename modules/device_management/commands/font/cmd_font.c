/*
 * cmd_font.c - `font [uzos|bios]`: show or change the console font.
 *
 * After switching, the text display copies the new glyphs into the
 * card (or the graphics console redraws with them): vga_reload().
 *
 * Uses: font_* (font/), vga_reload (vga/), ui_* (ui module).
 */

#include "device_commands.h"
#include "font.h"
#include "string.h"
#include "ui.h"
#include "box.h"

static const struct {
    font_id_t id;
    const char *about;
} fonts[] = {
    { FONT_UZOS, "clean and sharp (based on Terminus, SIL Open Font License)" },
    { FONT_BIOS, "the font the video card had at boot" },
};

void cmd_font(const char *args) {
    if (!args[0]) {
        ui_title("Fonts");
        for (unsigned i = 0; i < sizeof(fonts) / sizeof(fonts[0]); i++) {
            int active = fonts[i].id == font_current();
            ui_color(UI_GOOD);
            vga_print_str(active ? GLYPH_CHECK " " : "  ");
            ui_color(active ? UI_TITLE : VGA_COLOR_WHITE);
            ui_cell(font_name(fonts[i].id), 6);
            ui_color(UI_DIM);
            vga_print_str(fonts[i].about);
            ui_end();
        }
        ui_printf("  Sample: AaBbCc 0123 {}[] \xDA\xC4\xBF \xB0\xB1\xB2\xDB "
                  GLYPH_FOLDER GLYPH_FILE GLYPH_CHECK GLYPH_CROSS GLYPH_CLOCK GLYPH_CHIP
                  GLYPH_GEAR GLYPH_LOCK GLYPH_WARN GLYPH_INFO GLYPH_CRESCENT GLYPH_STAR "\n");
        ui_hint("font <name> switches.");
        return;
    }

    for (unsigned i = 0; i < sizeof(fonts) / sizeof(fonts[0]); i++) {
        if (strcmp(args, font_name(fonts[i].id)) != 0)
            continue;
        if (!font_select(fonts[i].id)) {
            ui_error("font", "that font is not available");
            return;
        }
        vga_reload();
        ui_start(UI_OK, "font");
        vga_print_str(font_name(fonts[i].id));
        ui_end();
        return;
    }
    ui_usage("font [uzos|bios]");
}

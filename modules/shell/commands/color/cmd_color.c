/*
 * cmd_color.c - `color <fg> [bg]`: change the normal text colour.
 *
 * The colour stays until the next `color`: every command goes back to
 * it when it is done (ui_reset). Colours that would make the text
 * invisible on its background are refused (contrast_check), measured
 * with the real colours of the current theme.
 *
 * Uses: ui_set_text_color (ui module), palette_rgb (device_management),
 *       def_contrast_ok (defence contrast_check), arg_next (lib/args).
 */

#include "shell_commands.h"
#include "args.h"
#include "string.h"
#include "ui.h"
#include "palette.h"
#include "def_contrast_check.h"

static const struct { const char *name; enum vga_color color; } colors[] = {
    { "black", VGA_COLOR_BLACK },          { "blue", VGA_COLOR_BLUE },
    { "green", VGA_COLOR_GREEN },          { "cyan", VGA_COLOR_CYAN },
    { "red", VGA_COLOR_RED },              { "magenta", VGA_COLOR_MAGENTA },
    { "brown", VGA_COLOR_BROWN },          { "grey", VGA_COLOR_LIGHT_GREY },
    { "darkgrey", VGA_COLOR_DARK_GREY },   { "lightblue", VGA_COLOR_LIGHT_BLUE },
    { "lightgreen", VGA_COLOR_LIGHT_GREEN }, { "lightcyan", VGA_COLOR_LIGHT_CYAN },
    { "lightred", VGA_COLOR_LIGHT_RED },   { "pink", VGA_COLOR_LIGHT_MAGENTA },
    { "yellow", VGA_COLOR_LIGHT_BROWN },   { "white", VGA_COLOR_WHITE },
};
#define COLOR_COUNT (sizeof(colors) / sizeof(colors[0]))

// The colour called 'name', or -1.
static int find_color(const char *name) {
    for (unsigned i = 0; i < COLOR_COUNT; i++)
        if (strcmp(colors[i].name, name) == 0)
            return colors[i].color;
    return -1;
}

static void usage(void) {
    ui_usage("color <fg> [bg]");
    for (unsigned i = 0; i < COLOR_COUNT; i++) {
        vga_print_str(" ");
        vga_set_color(colors[i].color, colors[i].color ? VGA_COLOR_BLACK : VGA_COLOR_LIGHT_GREY);
        vga_print_str(colors[i].name);
    }
    ui_end();
    ui_hint("  e.g. color white, color yellow blue, color white (back to normal)");
}

void cmd_color(const char *args) {
    char fg_name[16], bg_name[16];
    arg_next(arg_next(args, fg_name, sizeof(fg_name)), bg_name, sizeof(bg_name));

    int fg = find_color(fg_name);
    int bg = bg_name[0] ? find_color(bg_name) : VGA_COLOR_BLACK;
    if (fg < 0 || bg < 0) {
        usage();
        return;
    }
    if (!def_contrast_ok(palette_rgb(fg), palette_rgb(bg), "color: text too close to its background")) {
        ui_error("color", "refused: that text would be (nearly) invisible");
        return;
    }

    ui_set_text_color(fg, bg);
    ui_ok("color", "this is the new text colour");
}

#ifndef UI_H
#define UI_H

/*
 * ui.h - One look for every message in UzOS.
 *
 * Commands do not pick raw colours. They say what KIND of text it is,
 * and this file decides how that looks:
 *
 *     ui_error("rm", "cannot delete");      ✗ rm: cannot delete        (red)
 *     ui_warn("kill", "already gone");      ! kill: already gone        (yellow)
 *     ui_ok(0, "saved");                    ✓ saved                     (green)
 *     ui_usage("rm <path>");                i usage: rm <path>
 *
 * For a message built from several pieces:
 *
 *     ui_start(UI_ERROR, "cat");            ✗ cat:
 *     vga_print_str(path);                  ...anything...
 *     ui_end();                             end of the line, colour back
 *
 * The "normal text colour" can be changed with the `color` command;
 * ui_reset() always goes back to it.
 */

#include "vga.h"
#include "glyphs.h"

// Colours by meaning.
#define UI_TITLE    VGA_COLOR_LIGHT_CYAN        // headings, table headers
#define UI_LABEL    VGA_COLOR_LIGHT_GREY        // "Total :" in front of a value
#define UI_DIM      VGA_COLOR_DARK_GREY         // hints, frames, unimportant
#define UI_ACCENT   VGA_COLOR_LIGHT_MAGENTA     // things to notice
#define UI_GOOD     VGA_COLOR_LIGHT_GREEN
#define UI_WARNING  VGA_COLOR_LIGHT_BROWN
#define UI_BAD      VGA_COLOR_LIGHT_RED
#define UI_LINK     VGA_COLOR_LIGHT_BLUE        // directories, names to type

typedef enum { UI_OK, UI_INFO, UI_WARN, UI_ERROR } ui_kind_t;

/* ---------- Colours ---------- */

// The normal text colour (what `color` changes).
void ui_set_text_color(enum vga_color fg, enum vga_color bg);
enum vga_color ui_text_fg(void);
enum vga_color ui_text_bg(void);

void ui_reset(void);                    // back to the normal text colour
void ui_color(enum vga_color fg);       // 'fg' on the normal background

/* ---------- Messages: one whole line each ---------- */

// 'who' (the command name) may be 0.
void ui_ok(const char *who, const char *msg);
void ui_info(const char *who, const char *msg);
void ui_warn(const char *who, const char *msg);
void ui_error(const char *who, const char *msg);
void ui_usage(const char *usage);       // "usage: <usage>"

void ui_start(ui_kind_t kind, const char *who);
void ui_end(void);

/* ---------- Headings and text ---------- */

void ui_title(const char *title);       // a heading line
void ui_hint(const char *text);         // a dim line of help text
void ui_label(const char *label, int width);    // grey label, padded to 'width'

// Print with format() (lib/format): ui_printf("%d MB\n", mb).
void ui_printf(const char *fmt, ...);

#endif

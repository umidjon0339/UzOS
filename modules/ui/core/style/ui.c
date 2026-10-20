/*
 * ui.c - The message styles: a mark, a colour, and the normal colour back.
 *
 *   kind      mark  colour
 *   UI_OK      ✓    light green
 *   UI_INFO    i    light cyan
 *   UI_WARN    !    yellow
 *   UI_ERROR   ✗    light red
 *
 * Uses: vga_* (device_management), the icons GLYPH_* (font/glyphs.h),
 *       vformat (lib/format).
 */

#include "ui.h"
#include "format.h"
#include "string.h"

static enum vga_color text_fg = VGA_COLOR_WHITE;
static enum vga_color text_bg = VGA_COLOR_BLACK;

static const struct {
    const char *mark;
    enum vga_color color;
} kinds[] = {
    [UI_OK]    = { GLYPH_CHECK, UI_GOOD },
    [UI_INFO]  = { GLYPH_INFO,  UI_TITLE },
    [UI_WARN]  = { GLYPH_WARN,  UI_WARNING },
    [UI_ERROR] = { GLYPH_CROSS, UI_BAD },
};

/* ---------- Colours ---------- */

void ui_set_text_color(enum vga_color fg, enum vga_color bg) {
    text_fg = fg;
    text_bg = bg;
    ui_reset();
}

enum vga_color ui_text_fg(void) { return text_fg; }
enum vga_color ui_text_bg(void) { return text_bg; }

void ui_reset(void) {
    vga_set_color(text_fg, text_bg);
}

void ui_color(enum vga_color fg) {
    vga_set_color(fg, text_bg);
}

/* ---------- Messages ---------- */

void ui_start(ui_kind_t kind, const char *who) {
    ui_color(kinds[kind].color);
    vga_print_str(kinds[kind].mark);
    vga_print_str(" ");
    if (who) {
        vga_print_str(who);
        vga_print_str(": ");
    }
}

void ui_end(void) {
    vga_print_str("\n");
    ui_reset();
}

static void say(ui_kind_t kind, const char *who, const char *msg) {
    ui_start(kind, who);
    vga_print_str(msg);
    ui_end();
}

void ui_ok(const char *who, const char *msg)    { say(UI_OK, who, msg); }
void ui_info(const char *who, const char *msg)  { say(UI_INFO, who, msg); }
void ui_warn(const char *who, const char *msg)  { say(UI_WARN, who, msg); }
void ui_error(const char *who, const char *msg) { say(UI_ERROR, who, msg); }

void ui_usage(const char *usage) {
    ui_color(UI_TITLE);
    vga_print_str(GLYPH_INFO " ");
    ui_color(UI_DIM);
    vga_print_str("usage: ");
    ui_reset();
    vga_print_str(usage);
    vga_print_str("\n");
}

/* ---------- Headings and text ---------- */

void ui_title(const char *title) {
    ui_color(UI_ACCENT);
    vga_print_str(GLYPH_ARROW " ");
    ui_color(UI_TITLE);
    vga_print_str(title);
    ui_end();
}

void ui_hint(const char *text) {
    ui_color(UI_DIM);
    vga_print_str(text);
    ui_end();
}

void ui_label(const char *label, int width) {
    ui_color(UI_LABEL);
    vga_print_str(label);
    for (int n = strlen(label); n < width; n++)
        vga_put_char(' ');
    ui_reset();
}

void ui_printf(const char *fmt, ...) {
    char text[256];
    va_list args;
    va_start(args, fmt);
    vformat(text, sizeof(text), fmt, args);
    va_end(args);
    vga_print_str(text);
}

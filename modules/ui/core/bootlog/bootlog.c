/*
 * bootlog.c - Print "[  OK  ] step  detail" lines while the OS starts.
 *
 * Uses: vga_* (device_management), ui colours (core/style).
 */

#include "bootlog.h"
#include "ui.h"

#define NAME_WIDTH 26           // details start in the same column

static const char *step;        // the step that is running now
static int step_column;         // where the cursor was after "[ .... ] step"

static void result(const char *tag, enum vga_color color, const char *detail) {
    if (vga_column() != step_column && vga_column() != 0)  // the step printed something:
        vga_put_char('\n');                 // write the result on a fresh line
    vga_put_char('\r');                     // back over "[ .... ]"
    ui_color(UI_DIM);
    vga_print_str("[");
    ui_color(color);
    vga_print_str(tag);
    ui_color(UI_DIM);
    vga_print_str("] ");
    ui_label(step, NAME_WIDTH);             // written again, same place
    ui_color(UI_DIM);
    vga_print_str(detail);
    ui_end();
}

void boot_begin(const char *what) {
    step = what;
    ui_color(UI_DIM);
    vga_print_str("[ .... ] ");
    ui_color(UI_LABEL);
    vga_print_str(what);
    step_column = vga_column();
}

void boot_ok(const char *detail) {
    result("  OK  ", UI_GOOD, detail);
}

void boot_skip(const char *reason) {
    result(" SKIP ", UI_WARNING, reason);
}

void boot_fail(const char *reason) {
    result("FAILED", UI_BAD, reason);
    ui_color(UI_BAD);
    vga_print_str("System halted.\n");
    asm volatile("cli");
    while (1)
        asm volatile("hlt");
}

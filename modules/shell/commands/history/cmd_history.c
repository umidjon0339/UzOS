/*
 * cmd_history.c - `history`: the commands typed so far, oldest first.
 *
 * Uses: history_count / history_get (core/history), ui colours.
 */

#include "shell_commands.h"
#include "history.h"
#include "ui.h"

void cmd_history(const char *args) {
    (void)args;
    for (int i = 0; i < history_count(); i++) {
        ui_color(UI_DIM);
        ui_printf("%4d  ", i + 1);
        ui_reset();
        vga_print_str(history_get(i));
        vga_print_str("\n");
    }
}

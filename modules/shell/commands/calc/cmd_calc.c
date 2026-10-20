/*
 * cmd_calc.c - `calc <expression>`: integer calculator.
 *
 *   calc 2 + 3 * 4        = 14   (* before +)
 *   calc (2 + 3) * 4      = 20
 *   calc 17 % 5           = 2
 *
 * Uses: calc_eval (calc.c, next to this file), ui_* (ui module).
 */

#include "shell_commands.h"
#include "calc.h"
#include "ui.h"

void cmd_calc(const char *args) {
    if (!args[0]) {
        ui_usage("calc <expression>   e.g. calc (2 + 3) * 4");
        ui_hint("  operators: + - * / %  and ( )");
        return;
    }

    int32_t result;
    const char *error;
    if (!calc_eval(args, &result, &error)) {
        ui_error("calc", error);
        return;
    }

    ui_color(UI_DIM);
    vga_print_str("= ");
    ui_color(UI_GOOD);
    vga_print_int(result);
    ui_end();
}

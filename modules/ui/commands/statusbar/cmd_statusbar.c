/*
 * cmd_statusbar.c - `statusbar [on|off]`: show or hide the top bar.
 *
 * Uses: statusbar_* (core/statusbar), ui_* (core/style).
 */

#include "ui_commands.h"
#include "statusbar.h"
#include "string.h"
#include "ui.h"

void cmd_statusbar(const char *args) {
    if (!args[0]) {
        ui_info("statusbar", statusbar_shown() ? "on" : "off");
    } else if (strcmp(args, "on") == 0) {
        statusbar_show(1);
    } else if (strcmp(args, "off") == 0) {
        statusbar_show(0);
    } else {
        ui_usage("statusbar [on|off]");
    }
}

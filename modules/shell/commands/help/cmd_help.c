/*
 * cmd_help.c - `help [command]`: what commands there are.
 *
 *   help        every command with its one-line description, grouped by
 *               the module that added it:
 *
 *                 CPU:
 *                   cpuinfo    The CPU: name, features, control registers
 *                   faults     CPU exceptions that killed a process
 *
 *   help ls     the description of `ls` and its group
 *
 * The list is built from the registry, so it is never out of date.
 *
 * Uses: shell_command_count / shell_command / shell_find (core/shell),
 *       vga_cols (device_management) for the line width, ui_* and
 *       ui_cell (ui module).
 */

#include "shell_commands.h"
#include "shell.h"
#include "string.h"
#include "ui.h"
#include "box.h"

#define INDENT       "  "
#define NAME_WIDTH   10         // longest names ("statusbar", "testuser") + a space

static void explain(const char *name) {
    const shell_command_t *cmd = shell_find(name);
    if (!cmd) {
        ui_start(UI_ERROR, "help");
        ui_printf("no command called '%s'", name);
        ui_end();
        return;
    }
    ui_color(UI_TITLE);
    vga_print_str(cmd->name);
    ui_reset();
    vga_print_str(" - ");
    vga_print_str(cmd->description);
    ui_color(UI_DIM);
    ui_printf("  [%s]", cmd->group);
    ui_end();
}

// One line per command of 'group': its name, then its description, cut
// to the screen width so a long one never wraps.
static void list_group(const char *group) {
    int text_width = vga_cols() - 1 - (int)strlen(INDENT) - NAME_WIDTH;

    for (int i = 0; i < shell_command_count(); i++) {
        const shell_command_t *cmd = shell_command(i);
        if (strcmp(cmd->group, group) != 0)
            continue;

        vga_print_str(INDENT);
        ui_color(UI_LINK);
        ui_cell(cmd->name, NAME_WIDTH);
        ui_reset();
        ui_cell(cmd->description, text_width);
        vga_print_str("\n");
    }
}

// Has 'group' already appeared before command 'index'?
static int seen_before(int index, const char *group) {
    for (int i = 0; i < index; i++)
        if (strcmp(shell_command(i)->group, group) == 0)
            return 1;
    return 0;
}

void cmd_help(const char *args) {
    if (args[0]) {
        explain(args);
        return;
    }

    for (int i = 0; i < shell_command_count(); i++) {
        const char *group = shell_command(i)->group;
        if (seen_before(i, group))
            continue;
        ui_color(UI_TITLE);
        vga_print_str(group);
        vga_print_str(":\n");
        ui_reset();
        list_group(group);
    }

    ui_hint("help <command> explains one. Tab completes, Up/Down = history, wheel scrolls.");
}

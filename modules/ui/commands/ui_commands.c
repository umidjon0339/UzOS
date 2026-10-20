/*
 * ui_commands.c - Register the ui commands with the shell.
 *
 * Uses: shell_begin_group / shell_register_command (shell).
 */

#include "ui_commands.h"
#include "shell.h"

static const struct {
    const char *name;
    const char *description;
    void (*handler)(const char *args);
} commands[] = {
    { "uzfetch",   "System summary with the UzOS logo",      cmd_uzfetch   },
    { "statusbar", "Top status bar: statusbar [on|off]",     cmd_statusbar },
};

void register_ui_commands(void) {
    shell_begin_group("Screen");
    for (unsigned i = 0; i < sizeof(commands) / sizeof(commands[0]); i++)
        shell_register_command(commands[i].name, commands[i].description,
                               commands[i].handler);
}

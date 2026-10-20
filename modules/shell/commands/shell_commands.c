/*
 * shell_commands.c - Register the shell's own commands.
 *
 * Uses: shell_begin_group / shell_register_command (core/shell).
 */

#include "shell_commands.h"
#include "shell.h"

static const struct {
    const char *name;
    const char *description;
    void (*handler)(const char *args);
} commands[] = {
    { "help",    "List commands, or explain one: help [command]", cmd_help    },
    { "clear",   "Clear the screen",                           cmd_clear   },
    { "echo",    "Print text: echo <text>",                    cmd_echo    },
    { "color",   "Text colour: color <fg> [bg]",               cmd_color   },
    { "calc",    "Calculator: calc 2 + 3 * (4 - 1)",           cmd_calc    },
    { "history", "The last commands you typed",                cmd_history },
    { "snake",   "Play Snake (arrows/WASD, Q quits)",          cmd_snake   },
};

void register_shell_commands(void) {
    shell_begin_group("Shell");
    for (unsigned i = 0; i < sizeof(commands) / sizeof(commands[0]); i++)
        shell_register_command(commands[i].name, commands[i].description,
                               commands[i].handler);
}

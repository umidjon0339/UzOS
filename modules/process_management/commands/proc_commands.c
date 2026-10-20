/*
 * proc_commands.c - Register the process commands with the shell.
 *
 * Uses: shell_register_command (shell).
 */

#include "proc_commands.h"
#include "shell.h"

static const struct {
    const char *name;
    const char *description;
    void (*handler)(const char *args);
} commands[] = {
    { "ps",       "List running processes",                 cmd_ps       },
    { "top",      "Live view of processes ('q' quits)",     cmd_top      },
    { "kill",     "Kill a process by PID",                  cmd_kill     },
    { "spawn",    "Start a background worker process",      cmd_spawn    },
    { "sleep",    "Sleep for N seconds",                    cmd_sleep    },
    { "exec",     "Run an ELF program from the filesystem", cmd_exec     },
    { "testuser", "Ring 3 program: testuser [null|badptr]", cmd_testuser },
};

void register_proc_commands(void) {
    shell_begin_group("Processes");
    for (unsigned i = 0; i < sizeof(commands) / sizeof(commands[0]); i++)
        shell_register_command(commands[i].name, commands[i].description,
                               commands[i].handler);
}

/*
 * cpu_commands.c - Register the CPU commands with the shell.
 *
 * Uses: shell_register_command (shell).
 */

#include "cpu_commands.h"
#include "shell.h"

static const struct {
    const char *name;
    const char *description;
    void (*handler)(const char *args);
} commands[] = {
    { "cpuinfo", "The CPU: name, features, control registers: cpuinfo [flags]", cmd_cpuinfo },
    { "faults",  "CPU exceptions that killed a process: faults [list]",         cmd_faults  },
    { "cpustat", "CPU load: busy/idle, ring 0/3, time per process",             cmd_cpustat },
    { "nice",    "Priorities (1-7, 7 first): nice [pid [level]]",               cmd_nice    },
};

void register_cpu_commands(void) {
    shell_begin_group("CPU");
    for (unsigned i = 0; i < sizeof(commands) / sizeof(commands[0]); i++)
        shell_register_command(commands[i].name, commands[i].description,
                               commands[i].handler);
}

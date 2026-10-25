/*
 * security_commands.c - Register the security commands with the shell.
 *
 * Uses: shell_register_command (shell).
 */

#include "security_commands.h"
#include "shell.h"

static const struct {
    const char *name;
    const char *description;
    void (*handler)(const char *args);
} commands[] = {
    { "seclab", "Security lab: attacks and defences",          cmd_seclab },
    { "audit",  "What the defences refused, and to whom: audit [sum]", cmd_audit },
    { "caps",   "Which privileges a user holds: caps [user]",  cmd_caps   },
    { "integrity", "Is the kernel's code unchanged since boot?", cmd_integrity },
};

void register_security_commands(void) {
    shell_begin_group("Security");
    for (unsigned i = 0; i < sizeof(commands) / sizeof(commands[0]); i++)
        shell_register_command(commands[i].name, commands[i].description,
                               commands[i].handler);
}

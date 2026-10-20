/*
 * user_commands.c - Register the user commands with the shell.
 *
 * Uses: shell_register_command (shell).
 */

#include "user_commands.h"
#include "shell.h"

static const struct {
    const char *name;
    const char *description;
    void (*handler)(const char *args);
} commands[] = {
    { "whoami", "The user this shell runs as",                 cmd_whoami },
    { "id",     "A user's uid and home: id [user]",            cmd_id     },
    { "users",  "Every account: uid, home, processes, hash",   cmd_users  },
    { "su",     "Become another user: su [user] (default root)", cmd_su     },
    { "logout", "End the session, back to the login prompt",   cmd_logout },
    { "passwd", "Change a password: passwd [user]",            cmd_passwd },
    { "chmod",  "Who may read/write: chmod <mode> <path> (64 = rw/r-)", cmd_chmod },
    { "chown",  "Give a file to a user (root): chown <user> <path>", cmd_chown  },
    { "useradd", "Add an account (root): useradd <name>",        cmd_useradd },
    { "userdel", "Remove an account (root): userdel <name>",     cmd_userdel },
};

void register_user_commands(void) {
    shell_begin_group("Users");
    for (unsigned i = 0; i < sizeof(commands) / sizeof(commands[0]); i++)
        shell_register_command(commands[i].name, commands[i].description,
                               commands[i].handler);
}

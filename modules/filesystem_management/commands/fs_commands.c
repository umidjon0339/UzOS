/*
 * fs_commands.c - Register the filesystem commands with the shell.
 *
 * Uses: shell_register_command (shell).
 */

#include "fs_commands.h"
#include "shell.h"

static const struct {
    const char *name;
    const char *description;
    void (*handler)(const char *args);
} commands[] = {
    { "ls",      "List a directory: ls [path]",               cmd_ls      },
    { "cd",      "Change directory: cd [path] (none = home)", cmd_cd      },
    { "pwd",     "Print the current directory",               cmd_pwd     },
    { "mkdir",   "Create a directory: mkdir <path>",          cmd_mkdir   },
    { "touch",   "Create an empty file: touch <path>",        cmd_touch   },
    { "rm",      "Delete a file or empty directory",          cmd_rm      },
    { "cat",     "Print a file: cat <path>",                  cmd_cat     },
    { "write",   "Replace a file's text: write <path> <text>", cmd_write  },
    { "cp",      "Copy a file: cp <from> <to>",               cmd_cp      },
    { "mv",      "Move or rename: mv <from> <to>",            cmd_mv      },
    { "wc",      "Count lines, words, bytes: wc <path>",      cmd_wc      },
    { "grep",    "Lines containing a word: grep <word> <path>", cmd_grep  },
    { "hexdump", "Show a file as hex bytes: hexdump <path>",  cmd_hexdump },
};

void register_fs_commands(void) {
    shell_begin_group("Files");
    for (unsigned i = 0; i < sizeof(commands) / sizeof(commands[0]); i++)
        shell_register_command(commands[i].name, commands[i].description,
                               commands[i].handler);
}

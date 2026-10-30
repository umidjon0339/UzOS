/*
 * fs_commands.c - Register the filesystem commands with the shell.
 *
 * One row per command: the name you type, the line `help` shows, and
 * the function in commands/<name>/cmd_<name>.c that runs it.
 *
 * Uses: shell_begin_group / shell_register_command (shell), and every
 *       cmd_<name>() of this module.
 */

#include "modules/filesystem_management/commands/fs_commands.h"  // this file
#include "modules/shell/core/shell/shell.h"                      // shell_register_command

// Moving around and looking
#include "modules/filesystem_management/commands/ls/cmd_ls.h"
#include "modules/filesystem_management/commands/cd/cmd_cd.h"
#include "modules/filesystem_management/commands/pwd/cmd_pwd.h"
#include "modules/filesystem_management/commands/mkdir/cmd_mkdir.h"
#include "modules/filesystem_management/commands/touch/cmd_touch.h"
#include "modules/filesystem_management/commands/rm/cmd_rm.h"
#include "modules/filesystem_management/commands/cat/cmd_cat.h"
#include "modules/filesystem_management/commands/write/cmd_write.h"
#include "modules/filesystem_management/commands/cp/cmd_cp.h"
#include "modules/filesystem_management/commands/mv/cmd_mv.h"
#include "modules/filesystem_management/commands/wc/cmd_wc.h"
#include "modules/filesystem_management/commands/grep/cmd_grep.h"
#include "modules/filesystem_management/commands/hexdump/cmd_hexdump.h"
// Metadata
#include "modules/filesystem_management/commands/stat/cmd_stat.h"
#include "modules/filesystem_management/commands/umask/cmd_umask.h"
#include "modules/filesystem_management/commands/truncate/cmd_truncate.h"
// Trees
#include "modules/filesystem_management/commands/rmdir/cmd_rmdir.h"
#include "modules/filesystem_management/commands/tree/cmd_tree.h"
#include "modules/filesystem_management/commands/find/cmd_find.h"
#include "modules/filesystem_management/commands/du/cmd_du.h"
#include "modules/filesystem_management/commands/realpath/cmd_realpath.h"
#include "modules/filesystem_management/commands/basename/cmd_basename.h"
#include "modules/filesystem_management/commands/dirname/cmd_dirname.h"
// Links
#include "modules/filesystem_management/commands/ln/cmd_ln.h"
#include "modules/filesystem_management/commands/readlink/cmd_readlink.h"
#include "modules/filesystem_management/commands/unlink/cmd_unlink.h"
// Testing the syscalls
#include "modules/filesystem_management/commands/fscalls/cmd_fscalls.h"

static const struct {
    const char *name;
    const char *description;
    void (*handler)(const char *args);
} commands[] = {
    { "ls",      "List a directory: ls [path]",               cmd_ls      },
    { "cd",      "Change directory: cd [path] (none = home)", cmd_cd      },
    { "pwd",     "Print the current directory",               cmd_pwd     },
    { "mkdir",   "Create a directory: mkdir <path>",          cmd_mkdir   },
    { "touch",   "Create a file / set its times: touch [-r ref] <path>", cmd_touch },
    { "rm",      "Delete: rm [-r] <path>...",                 cmd_rm      },
    { "rmdir",   "Remove empty directories: rmdir <dir>...",  cmd_rmdir   },
    { "tree",    "Draw a directory tree: tree [path]",        cmd_tree    },
    { "find",    "Search a tree: find [path] [-name x] [-type f|d]", cmd_find },
    { "du",      "Bytes per directory: du [-s] [path]",       cmd_du      },
    { "realpath", "Where a path really leads: realpath <path>", cmd_realpath },
    { "basename", "Last name of a path: basename <path>",     cmd_basename },
    { "dirname", "A path without its last name: dirname <path>", cmd_dirname },
    { "cat",     "Print a file: cat <path>",                  cmd_cat     },
    { "write",   "Replace a file's text: write <path> <text>", cmd_write  },
    { "cp",      "Copy a file: cp <from> <to>",               cmd_cp      },
    { "mv",      "Move or rename: mv <from> <to>",            cmd_mv      },
    { "wc",      "Count lines, words, bytes: wc <path>",      cmd_wc      },
    { "grep",    "Lines containing a word: grep <word> <path>", cmd_grep  },
    { "hexdump", "Show a file as hex bytes: hexdump <path>",  cmd_hexdump },
    { "stat",    "Inode, links, owner, rights, times: stat <path>", cmd_stat },
    { "umask",   "Bits new files leave out: umask [022]",      cmd_umask   },
    { "truncate", "Set a file's length: truncate -s SIZE <path>", cmd_truncate },
    { "ln",      "Another name: ln [-s] <target> <name>",     cmd_ln      },
    { "readlink", "Where a symbolic link points: readlink <link>", cmd_readlink },
    { "unlink",  "Remove one name of a file: unlink <file>",  cmd_unlink  },
    { "fscalls", "Test: a ring 3 program calls the file syscalls", cmd_fscalls },
};

void register_fs_commands(void) {
    shell_begin_group("Files");
    for (unsigned i = 0; i < sizeof(commands) / sizeof(commands[0]); i++)
        shell_register_command(commands[i].name, commands[i].description,
                               commands[i].handler);
}

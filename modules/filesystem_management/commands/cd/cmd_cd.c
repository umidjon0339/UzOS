/*
 * cmd_cd.c - `cd [path]`: change directory. Without a path: go to /.
 *
 * Uses: sys_chdir (syscalls/chdir), ui_* (ui module).
 */

#include "fs_commands.h"
#include "sys_chdir.h"
#include "ui.h"

void cmd_cd(const char *args) {
    if (sys_chdir(args[0] ? args : "/") != 0)
        ui_error("cd", "no such directory");
}

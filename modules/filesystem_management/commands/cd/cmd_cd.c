/*
 * cmd_cd.c - `cd [path]`: change directory. Without a path: go home.
 *
 * Uses: sys_chdir (syscalls/chdir), users_find_uid (user_management) for
 *       the home directory, process_caller_uid (process_management),
 *       ui_* (ui module).
 */

#include "fs_commands.h"
#include "sys_chdir.h"
#include "users.h"
#include "process.h"
#include "ui.h"

void cmd_cd(const char *args) {
    const user_t *me = users_find_uid(process_caller_uid());
    const char *path = args[0] ? args : me ? me->home : "/";

    if (sys_chdir(path) != 0)
        ui_error("cd", "no such directory, or permission denied");
}

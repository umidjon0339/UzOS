/*
 * cmd_cd.c - `cd [path]`: change directory. Without a path: go home.
 *
 * Uses: sys_chdir (syscalls/chdir), users_find_uid (user_management) for
 *       the home directory, process_caller_uid (process_management),
 *       ui_* (ui module).
 */

#include "modules/filesystem_management/commands/cd/cmd_cd.h"        // this file
#include "modules/filesystem_management/syscalls/chdir/sys_chdir.h"  // sys_chdir
#include "modules/user_management/core/users/users.h"                // user_t, users_find_uid
#include "modules/process_management/core/process/process.h"         // process_caller_uid
#include "modules/ui/core/style/ui.h"                                // ui_error

void cmd_cd(const char *args) {
    const user_t *me = users_find_uid(process_caller_uid());
    const char *path = args[0] ? args : me ? me->home : "/";

    if (sys_chdir(path) != 0)
        ui_error("cd", "no such directory, or permission denied");
}

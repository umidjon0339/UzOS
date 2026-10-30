/*
 * cmd_mkdir.c - `mkdir <path>`: create a directory.
 *
 * Uses: sys_mkdir (syscalls/mkdir), ui_* (ui module).
 */

#include "modules/filesystem_management/commands/mkdir/cmd_mkdir.h"  // this file
#include "modules/filesystem_management/syscalls/mkdir/sys_mkdir.h"  // sys_mkdir
#include "modules/ui/core/style/ui.h"                                // ui_usage, ui_error

void cmd_mkdir(const char *args) {
    if (!args[0]) {
        ui_usage("mkdir <path>");
        return;
    }
    if (sys_mkdir(args) != 0)
        ui_error("mkdir", "cannot create (exists, bad name, no such parent, or permission denied)");
}

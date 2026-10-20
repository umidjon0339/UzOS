/*
 * cmd_mkdir.c - `mkdir <path>`: create a directory.
 *
 * Uses: sys_mkdir (syscalls/mkdir), ui_* (ui module).
 */

#include "fs_commands.h"
#include "sys_mkdir.h"
#include "ui.h"

void cmd_mkdir(const char *args) {
    if (!args[0]) {
        ui_usage("mkdir <path>");
        return;
    }
    if (sys_mkdir(args) != 0)
        ui_error("mkdir", "cannot create (exists, bad name, or no such parent)");
}

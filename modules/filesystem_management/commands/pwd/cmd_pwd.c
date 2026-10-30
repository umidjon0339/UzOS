/*
 * cmd_pwd.c - `pwd`: print the current directory.
 *
 * Uses: sys_getcwd (syscalls/getcwd), ui_* (ui module).
 */

#include "modules/filesystem_management/commands/pwd/cmd_pwd.h"        // this file
#include "modules/filesystem_management/syscalls/getcwd/sys_getcwd.h"  // sys_getcwd
#include "modules/filesystem_management/core/vfs/vfs.h"                // FS_PATH_MAX
#include "modules/ui/core/style/ui.h"                                  // ui_error

void cmd_pwd(const char *args) {
    (void)args;
    char path[FS_PATH_MAX];

    if (sys_getcwd(path, sizeof(path)) == 0) {
        vga_print_str(path);
        vga_print_str("\n");
    } else {
        ui_error("pwd", "path too long");
    }
}

/*
 * cmd_pwd.c - `pwd`: print the current directory.
 *
 * Uses: sys_getcwd (syscalls/getcwd), ui_* (ui module).
 */

#include "fs_commands.h"
#include "sys_getcwd.h"
#include "vfs.h"
#include "ui.h"

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

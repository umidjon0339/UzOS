/*
 * cmd_touch.c - `touch <path>`: create an empty file if it does not exist.
 *
 * Uses: sys_open with O_CREATE, sys_close (io_management), ui_* (ui module).
 */

#include "fs_commands.h"
#include "sys_open.h"
#include "sys_close.h"
#include "ui.h"

void cmd_touch(const char *args) {
    if (!args[0]) {
        ui_usage("touch <path>");
        return;
    }
    int fd = sys_open(args, O_WRITE | O_CREATE);
    if (fd < 0) {
        ui_error("touch", "cannot create (bad name, no such directory, or permission denied)");
        return;
    }
    sys_close(fd);
}

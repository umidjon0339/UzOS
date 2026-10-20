/*
 * cmd_rm.c - `rm <path>`: delete a file or an empty directory.
 *
 * Uses: sys_unlink (syscalls/unlink), ui_* (ui module).
 * Refusals are explained in `seclab log`.
 */

#include "fs_commands.h"
#include "sys_unlink.h"
#include "ui.h"

void cmd_rm(const char *args) {
    if (!args[0]) {
        ui_usage("rm <path>");
        return;
    }
    if (sys_unlink(args) != 0)
        ui_error("rm", "cannot delete (not found, open, not empty, or in use)");
}

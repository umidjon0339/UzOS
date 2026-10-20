/*
 * cmd_mv.c - `mv <from> <to>`: move or rename.
 *
 * Uses: sys_rename (syscalls/rename), arg_next (lib/args), ui_* (ui module).
 */

#include "fs_commands.h"
#include "sys_rename.h"
#include "args.h"
#include "vfs.h"
#include "ui.h"

void cmd_mv(const char *args) {
    char from[FS_PATH_MAX], to[FS_PATH_MAX];
    arg_next(arg_next(args, from, sizeof(from)), to, sizeof(to));

    if (!from[0] || !to[0]) {
        ui_usage("mv <from> <to>");
        return;
    }
    if (sys_rename(from, to) != 0)
        ui_error("mv", "refused (not found, name taken, bad name, a loop, or permission denied)");
}

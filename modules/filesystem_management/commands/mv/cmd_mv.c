/*
 * cmd_mv.c - `mv <from> <to>`: move or rename.
 *
 * Uses: sys_rename (syscalls/rename), arg_next (lib/args), ui_* (ui module).
 */

#include "modules/filesystem_management/commands/mv/cmd_mv.h"          // this file
#include "modules/filesystem_management/syscalls/rename/sys_rename.h"  // sys_rename
#include "lib/args/args.h"                                             // arg_next
#include "modules/filesystem_management/core/vfs/vfs.h"                // FS_PATH_MAX
#include "modules/ui/core/style/ui.h"                                  // ui_usage, ui_error

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

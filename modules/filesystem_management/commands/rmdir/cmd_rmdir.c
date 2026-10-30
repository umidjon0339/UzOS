/*
 * cmd_rmdir.c - `rmdir <dir>...`: remove empty directories.
 *
 * Only empty ones (busy_check); a file is refused - that is `rm`'s job.
 *
 * Uses: sys_rmdir (syscalls/rmdir), arg_next, ui.
 */
#include "modules/filesystem_management/commands/rmdir/cmd_rmdir.h"    // this file
#include "modules/filesystem_management/syscalls/rmdir/sys_rmdir.h"    // sys_rmdir
#include "modules/filesystem_management/core/vfs/vfs.h"                // FS_PATH_MAX
#include "lib/args/args.h"                                             // arg_next
#include "modules/ui/core/style/ui.h"                                  // ui_usage, ui_printf

void cmd_rmdir(const char *args) {
    char word[FS_PATH_MAX];
    const char *rest = arg_next(args, word, sizeof(word));
    if (!word[0]) {
        ui_usage("rmdir <dir>...");
        return;
    }
    for (; word[0]; rest = arg_next(rest, word, sizeof(word)))
        if (sys_rmdir(word) != 0)
            ui_printf("rmdir: %s: not an empty directory of yours (see `audit`)\n", word);
}

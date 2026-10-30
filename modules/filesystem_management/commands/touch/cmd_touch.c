/*
 * cmd_touch.c - `touch [-r ref] <path>`: create a file if needed, and
 * set its times.
 *
 *   touch notes          create it empty, or set its times to now
 *   touch -r old notes   give notes the same times as old
 *
 * "Now" is allowed to the owner and to anyone who may write the file;
 * copying ANOTHER file's times is choosing a time: owner only
 * (time_owner).
 *
 * Uses: sys_open with O_CREATE, sys_close (io_management), sys_utimensat
 *       and sys_stat (syscalls/), ui_* (ui module).
 */

#include "modules/filesystem_management/commands/touch/cmd_touch.h"          // this file
#include "modules/io_management/syscalls/open/sys_open.h"                    // sys_open, O_WRITE, O_CREATE
#include "modules/io_management/syscalls/close/sys_close.h"                  // sys_close
#include "modules/filesystem_management/syscalls/stat/sys_stat.h"            // sys_stat, vfs_stat_t
#include "modules/filesystem_management/syscalls/utimensat/sys_utimensat.h"  // sys_utimensat, utimensat_ts_t
#include "lib/args/args.h"                                                   // arg_next
#include "lib/string/string.h"                                               // strcmp
#include "modules/ui/core/style/ui.h"                                        // ui_usage, ui_error

void cmd_touch(const char *args) {
    char word[FS_PATH_MAX], ref[FS_PATH_MAX] = "";
    const char *rest = arg_next(args, word, sizeof(word));
    if (strcmp(word, "-r") == 0) {
        rest = arg_next(rest, ref, sizeof(ref));
        arg_next(rest, word, sizeof(word));
    }
    if (!word[0]) {
        ui_usage("touch [-r ref] <path>");
        return;
    }

    utimensat_ts_t times[2];
    if (ref[0]) {                                   // the reference file's times
        vfs_stat_t st;
        if (sys_stat(ref, &st) != 0) {
            ui_error("touch", "cannot read the reference file");
            return;
        }
        times[0].sec = st.atime;  times[0].nsec = 0;
        times[1].sec = st.mtime;  times[1].nsec = 0;
    }

    int fd = sys_open(word, O_WRITE | O_CREATE);
    if (fd >= 0) {
        sys_close(fd);
    } else if (sys_stat(word, &(vfs_stat_t){0}) != 0) {
        ui_error("touch", "cannot create (bad name, no such directory, or permission denied)");
        return;
    }
    if (sys_utimensat(AT_FDCWD, word, ref[0] ? times : 0, 0) != 0)
        ui_error("touch", "cannot set its times (see `audit`)");
}

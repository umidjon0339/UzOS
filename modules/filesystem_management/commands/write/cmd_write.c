/*
 * cmd_write.c - `write <path> <text>`: replace a file's content.
 *
 * O_TRUNC empties the file first, so writing "hi" over "hello" gives
 * "hi", not "hillo". The file is created if it does not exist.
 *
 * Uses: sys_open / sys_write / sys_close (io_management), arg_next (lib/args),
 *       ui_* (ui module).
 */

#include "modules/filesystem_management/commands/write/cmd_write.h"  // this file
#include "modules/io_management/syscalls/open/sys_open.h"            // sys_open, O_WRITE, O_CREATE
#include "modules/io_management/syscalls/write/sys_write.h"          // sys_write
#include "modules/io_management/syscalls/close/sys_close.h"          // sys_close
#include "lib/args/args.h"                                           // arg_next
#include "lib/string/string.h"                                       // strlen
#include "modules/filesystem_management/core/vfs/vfs.h"              // FS_PATH_MAX
#include "modules/ui/core/style/ui.h"                                // ui_usage, ui_error

void cmd_write(const char *args) {
    char path[FS_PATH_MAX];
    const char *text = arg_next(args, path, sizeof(path));

    if (!path[0] || !text[0]) {
        ui_usage("write <path> <text>");
        return;
    }
    int fd = sys_open(path, O_WRITE | O_CREATE | O_TRUNC);
    if (fd < 0) {
        ui_error("write", "cannot open (bad name, no such directory, or permission denied)");
        return;
    }
    if (sys_write(fd, text, strlen(text)) < 0)
        ui_error("write", "failed");
    sys_close(fd);
}

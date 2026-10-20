/*
 * cmd_write.c - `write <path> <text>`: replace a file's content.
 *
 * O_TRUNC empties the file first, so writing "hi" over "hello" gives
 * "hi", not "hillo". The file is created if it does not exist.
 *
 * Uses: sys_open / sys_write / sys_close (io_management), arg_next (lib/args),
 *       ui_* (ui module).
 */

#include "fs_commands.h"
#include "sys_open.h"
#include "sys_write.h"
#include "sys_close.h"
#include "args.h"
#include "string.h"
#include "vfs.h"
#include "ui.h"

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

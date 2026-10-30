/*
 * cmd_readlink.c - `readlink <link>...`: the text a symbolic link holds.
 *
 *   ln -s ../docs d ; readlink d     ->  ../docs
 *
 * The link is not followed: this is the path it stores, which may lead
 * nowhere. (`realpath` follows it and says where it really ends.)
 *
 * Uses: sys_readlink (syscalls/readlink), arg_next, ui.
 */
#include "modules/filesystem_management/commands/readlink/cmd_readlink.h"   // this file
#include "modules/filesystem_management/syscalls/readlink/sys_readlink.h"   // sys_readlink
#include "modules/filesystem_management/core/vfs/vfs.h"                     // FS_PATH_MAX
#include "lib/args/args.h"                                                  // arg_next
#include "modules/ui/core/style/ui.h"                                       // ui_printf, ui_usage

void cmd_readlink(const char *args) {
    char path[FS_PATH_MAX], text[FS_PATH_MAX];
    const char *rest = arg_next(args, path, sizeof(path));
    if (!path[0]) {
        ui_usage("readlink <link>...");
        return;
    }
    for (; path[0]; rest = arg_next(rest, path, sizeof(path))) {
        int n = sys_readlink(path, text, sizeof(text) - 1);
        if (n < 0) {
            ui_printf("readlink: %s: not a symbolic link\n", path);
            continue;
        }
        text[n] = '\0';
        ui_printf("%s\n", text);
    }
}

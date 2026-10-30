/*
 * cmd_unlink.c - `unlink <file>`: remove one name, exactly as the syscall does.
 *
 * Unlike `rm` there are no options and no directories. The FILE goes away
 * only with its last name (and last open descriptor): `ln a b ; unlink a`
 * leaves the file, as b (inode_refcount).
 *
 * Uses: sys_unlink (syscalls/unlink), ui.
 */
#include "modules/filesystem_management/commands/unlink/cmd_unlink.h"   // this file
#include "modules/filesystem_management/syscalls/unlink/sys_unlink.h"   // sys_unlink
#include "modules/ui/core/style/ui.h"                                   // ui_usage, ui_error

void cmd_unlink(const char *args) {
    if (!args[0]) {
        ui_usage("unlink <file>");
        return;
    }
    if (sys_unlink(args) != 0)
        ui_error("unlink", "cannot remove it (no such file, a directory, in use, or not yours)");
}

/*
 * cmd_chmod.c - `chmod <mode> <path>`: who may read and write a file.
 *
 *   chmod 60 notes      only I may read and write it      rw/--
 *   chmod 64 notes      others may read it too            rw/r-
 *   chmod 40 notes      read-only, even for me            r-/--
 *
 * Each digit is 4 (read) + 2 (write): the first is for the owner, the
 * second for everybody else. Only the owner and root may change it.
 *
 * Uses: perm_parse / perm_may_chmod / perm_format (core/perm), vfs_lookup
 *       (filesystem_management), arg_next (lib), ui_* (ui module).
 */

#include "user_commands.h"
#include "perm.h"
#include "args.h"
#include "ui.h"

void cmd_chmod(const char *args) {
    char text[8];
    const char *path = arg_next(args, text, sizeof(text));
    uint32_t mode;

    if (!text[0] || !path[0]) {
        ui_usage("chmod <mode> <path>   e.g. chmod 60 notes");
        return;
    }
    if (!perm_parse(text, &mode)) {
        ui_error("chmod", "a mode is two digits from 0, 2, 4, 6 (4 = read, 2 = write)");
        return;
    }

    vfs_node_t *node = vfs_lookup(vfs_cwd(), path);
    if (!node) {
        ui_error("chmod", "no such file or directory");
        return;
    }
    if (!perm_may_chmod(node)) {
        ui_error("chmod", "permission denied: only the owner or root");
        return;
    }

    node->mode = mode;
    char shown[6];
    perm_format(mode, shown);
    ui_start(UI_OK, "chmod");
    ui_printf("%s is now %s", path, shown);
    ui_end();
}

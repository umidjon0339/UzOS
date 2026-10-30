/*
 * cmd_ln.c - `ln [-s] <target> <name>`: another name for a file.
 *
 *   ln notes copy        a HARD link: "copy" and "notes" are one file
 *                        (`stat` shows Links: 2; edit one, both change)
 *   ln -s /docs here     a SYMBOLIC link: "here" holds the text "/docs"
 *                        and every path through it continues there
 *
 * Hard links: only your own files, never directories (protected_hardlinks),
 * at most 16 names (nlink_bounds). Symbolic links point anywhere, even
 * at nothing yet.
 *
 * Uses: sys_link / sys_symlink (syscalls/), arg_next, strcmp, ui.
 */
#include "modules/filesystem_management/commands/ln/cmd_ln.h"           // this file
#include "modules/filesystem_management/syscalls/link/sys_link.h"       // sys_link
#include "modules/filesystem_management/syscalls/symlink/sys_symlink.h" // sys_symlink
#include "modules/filesystem_management/core/vfs/vfs.h"                 // FS_PATH_MAX
#include "lib/args/args.h"                                              // arg_next
#include "lib/string/string.h"                                          // strcmp
#include "modules/ui/core/style/ui.h"                                   // ui_usage, ui_error

void cmd_ln(const char *args) {
    char target[FS_PATH_MAX], name[FS_PATH_MAX];
    int symbolic = 0;
    const char *rest = arg_next(args, target, sizeof(target));
    if (strcmp(target, "-s") == 0) {
        symbolic = 1;
        rest = arg_next(rest, target, sizeof(target));
    }
    arg_next(rest, name, sizeof(name));
    if (!target[0] || !name[0]) {
        ui_usage("ln [-s] <target> <name>");
        return;
    }
    if ((symbolic ? sys_symlink(target, name) : sys_link(target, name)) != 0)
        ui_error("ln", symbolic ? "cannot make the link (name taken, or no permission)"
                                : "refused: not your file, a directory, too many names, or the name is taken (see `audit`)");
}

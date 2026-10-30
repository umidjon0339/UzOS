/*
 * cmd_realpath.c - `realpath <path>...`: where a path really leads.
 *
 *   cd /home/guest ; realpath ../root/./x/..    ->  /home/root
 *
 * The path is walked (".", "..", and - when UzOS has them - symbolic
 * links), then the node it reached prints its own absolute path.
 *
 * Uses: vfs_lookup / vfs_path_of / vfs_cwd (core/vfs), arg_next, ui.
 */
#include "modules/filesystem_management/commands/realpath/cmd_realpath.h"   // this file
#include "modules/filesystem_management/core/vfs/vfs.h"                    // vfs_lookup, vfs_path_of
#include "lib/args/args.h"                                                 // arg_next
#include "modules/ui/core/style/ui.h"                                      // ui_printf, ui_usage

void cmd_realpath(const char *args) {
    char word[FS_PATH_MAX], out[FS_PATH_MAX];
    const char *rest = arg_next(args, word, sizeof(word));
    if (!word[0]) {
        ui_usage("realpath <path>...");
        return;
    }
    for (; word[0]; rest = arg_next(rest, word, sizeof(word))) {
        vfs_node_t *node = vfs_lookup(vfs_cwd(), word);
        if (node && vfs_path_of(node, out, sizeof(out)) == 0)
            ui_printf("%s\n", out);
        else
            ui_printf("realpath: %s: no such file or directory\n", word);
    }
}

/*
 * cmd_du.c - `du [-s] [path]`: the bytes each directory holds.
 *
 *   du /home          one line per directory, its subdirectories included
 *   du -s /home       only the total
 *
 * Linux counts disk blocks; UzOS shows bytes (file sizes added up).
 * Directories you may not read count as 0 and are not entered; depth
 * stops at depth_limit (32).
 *
 * Uses: vfs_lookup / vfs_readdir / vfs_cwd (core/vfs), perm_check
 *       (user_management), FS_DEPTH_MAX (security/deep_nesting/defence/
 *       depth_limit), arg_next, strcmp / strlen / strcpy, ui.
 */
#include "modules/filesystem_management/commands/du/cmd_du.h"          // this file
#include "modules/filesystem_management/core/vfs/vfs.h"                // vfs_lookup, vfs_readdir
#include "modules/user_management/core/perm/perm.h"                    // perm_check, PERM_READ
#include "modules/filesystem_management/security/deep_nesting/defence/depth_limit/def_depth_limit.h"  // FS_DEPTH_MAX
#include "lib/args/args.h"                                             // arg_next
#include "lib/string/string.h"                                         // strcmp, strlen, strcpy
#include "modules/ui/core/style/ui.h"                                  // ui_printf, ui_usage

static char path[FS_PATH_MAX];
static int summary_only;

static uint32_t size_of(vfs_node_t *node, int depth) {
    if (!(node->flags & FS_DIRECTORY))
        return node->target ? 0 : node->size;      // a hard link's bytes count once, at the file
    uint32_t total = 0, len = strlen(path);
    if (depth < FS_DEPTH_MAX && perm_check(node, PERM_READ))
        for (uint32_t i = 0; ; i++) {
            vfs_node_t *child = vfs_readdir(node, i);
            if (!child)
                break;
            if (len + 1 + strlen(child->name) >= FS_PATH_MAX)
                continue;
            path[len] = '/';
            strcpy(path + len + 1, child->name);
            total += size_of(child, depth + 1);
            path[len] = '\0';
        }
    if (!summary_only || depth == 0)
        ui_printf("%-8u %s\n", total, path[0] ? path : "/");
    return total;
}

void cmd_du(const char *args) {
    char word[FS_PATH_MAX];
    const char *rest = arg_next(args, word, sizeof(word));
    summary_only = strcmp(word, "-s") == 0;
    if (summary_only)
        arg_next(rest, word, sizeof(word));
    if (!word[0])
        strcpy(word, ".");
    vfs_node_t *top = vfs_lookup(vfs_cwd(), word);
    if (!top) {
        ui_error("du", "no such file or directory");
        return;
    }
    strcpy(path, strcmp(word, "/") == 0 ? "" : word);
    uint32_t total = size_of(top, 0);
    if (!(top->flags & FS_DIRECTORY))
        ui_printf("%-8u %s\n", total, word);
}

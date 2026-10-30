/*
 * cmd_tree.c - `tree [path]`: draw a directory and everything below it.
 *
 *   /home
 *   |-- guest
 *   |   `-- notes
 *   `-- root
 *       `-- secret       (shown only if you may read root's directory)
 *   3 directories, 2 files
 *
 * A directory you may not read is shown, but not entered (perm_check:
 * the same rule as `ls`). Recursion is bounded by depth_limit (32), and
 * each level keeps only a byte of state ('last_at').
 *
 * Uses: vfs_lookup / vfs_readdir / vfs_cwd (core/vfs), perm_check
 *       (user_management), FS_DEPTH_MAX (security/deep_nesting/defence/
 *       depth_limit), ui.
 */
#include "modules/filesystem_management/commands/tree/cmd_tree.h"      // this file
#include "modules/filesystem_management/core/vfs/vfs.h"                // vfs_lookup, vfs_readdir
#include "modules/user_management/core/perm/perm.h"                    // perm_check, PERM_READ
#include "modules/filesystem_management/security/deep_nesting/defence/depth_limit/def_depth_limit.h"  // FS_DEPTH_MAX
#include "modules/ui/core/style/ui.h"                                  // ui_printf, ui_color

static char last_at[FS_DEPTH_MAX + 2];      // 1 = this level's entry is its directory's last
static uint32_t dirs, files;

static void draw(vfs_node_t *dir, int depth) {
    if (depth > FS_DEPTH_MAX || !perm_check(dir, PERM_READ))
        return;
    uint32_t count = 0;
    while (vfs_readdir(dir, count))
        count++;
    for (uint32_t i = 0; i < count; i++) {
        vfs_node_t *child = vfs_readdir(dir, i);
        for (int d = 0; d < depth; d++)
            ui_printf(last_at[d] ? "    " : "|   ");
        last_at[depth] = i == count - 1;
        ui_printf(last_at[depth] ? "`-- " : "|-- ");
        if (child->flags & FS_DIRECTORY) {
            ui_color(UI_ACCENT);
            ui_printf("%s\n", child->name);
            ui_reset();
            dirs++;
            draw(child, depth + 1);
        } else {
            ui_printf("%s\n", child->name);
            files++;
        }
    }
}

void cmd_tree(const char *args) {
    vfs_node_t *top = vfs_lookup(vfs_cwd(), args[0] ? args : ".");
    if (!top || !(top->flags & FS_DIRECTORY)) {
        ui_error("tree", "no such directory");
        return;
    }
    dirs = files = 0;
    ui_printf("%s\n", args[0] ? args : ".");
    draw(top, 0);
    ui_printf("%u directories, %u files\n", dirs, files);
}

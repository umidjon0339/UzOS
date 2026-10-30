/*
 * cmd_find.c - `find [path] [-name piece] [-type f|d]`: search a tree.
 *
 *   find /home -name notes      every path below /home whose name contains "notes"
 *   find . -type d              every directory below here
 *
 * Linux's -name takes a shell pattern; UzOS's matches a piece of the
 * name. Directories you may not read are not entered (perm_check), and
 * the walk stops at depth_limit (32). One shared path buffer: each level
 * costs a few bytes of kernel stack.
 *
 * Uses: vfs_lookup / vfs_readdir / vfs_cwd (core/vfs), perm_check
 *       (user_management), FS_DEPTH_MAX (security/deep_nesting/defence/
 *       depth_limit), arg_next, strcmp / strncmp / strlen / strcpy, ui.
 */
#include "modules/filesystem_management/commands/find/cmd_find.h"      // this file
#include "modules/filesystem_management/core/vfs/vfs.h"                // vfs_lookup, vfs_readdir
#include "modules/user_management/core/perm/perm.h"                    // perm_check, PERM_READ
#include "modules/filesystem_management/security/deep_nesting/defence/depth_limit/def_depth_limit.h"  // FS_DEPTH_MAX
#include "lib/args/args.h"                                             // arg_next
#include "lib/string/string.h"                                         // strcmp, strncmp, strlen
#include "modules/ui/core/style/ui.h"                                  // ui_printf, ui_usage

static char path[FS_PATH_MAX];
static char piece[FS_NAME_MAX + 1];
static char type;                   // 'f', 'd' or 0 = both
static uint32_t found;

static int contains(const char *name, const char *p) {
    int n = strlen(p);
    for (; *name; name++)
        if (strncmp(name, p, n) == 0)
            return 1;
    return !p[0];
}

static void visit(vfs_node_t *node, int depth) {
    int is_dir = (node->flags & FS_DIRECTORY) != 0;
    if ((!type || (type == 'd') == is_dir) && contains(node->name, piece)) {
        ui_printf("%s\n", path);
        found++;
    }
    if (!is_dir || depth >= FS_DEPTH_MAX || !perm_check(node, PERM_READ))
        return;
    uint32_t len = strlen(path);
    uint32_t at = len && path[len - 1] == '/' ? len : len + 1;    // "/" + name, or "a" + "/" + name
    for (uint32_t i = 0; ; i++) {
        vfs_node_t *child = vfs_readdir(node, i);
        if (!child)
            break;
        if (at + strlen(child->name) >= FS_PATH_MAX)
            continue;
        path[at - 1] = '/';
        strcpy(path + at, child->name);
        visit(child, depth + 1);
        path[len] = '\0';
    }
}

void cmd_find(const char *args) {
    char word[FS_PATH_MAX], start[FS_PATH_MAX] = ".";
    piece[0] = '\0';
    type = 0;
    for (const char *rest = arg_next(args, word, sizeof(word)); word[0];
         rest = arg_next(rest, word, sizeof(word))) {
        if (strcmp(word, "-name") == 0) {
            rest = arg_next(rest, piece, sizeof(piece));
        } else if (strcmp(word, "-type") == 0) {
            rest = arg_next(rest, word, sizeof(word));
            type = word[0] == 'f' || word[0] == 'd' ? word[0] : 0;
        } else if (word[0] == '-') {
            ui_usage("find [path] [-name piece] [-type f|d]");
            return;
        } else {
            strcpy(start, word);
        }
    }
    vfs_node_t *top = vfs_lookup(vfs_cwd(), start);
    if (!top) {
        ui_error("find", "no such file or directory");
        return;
    }
    found = 0;
    strcpy(path, start);
    visit(top, 0);
    if (!found)
        ui_printf("(nothing found)\n");
}

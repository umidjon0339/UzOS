/*
 * cmd_rm.c - `rm [-r] <path>...`: delete.
 *
 *   rm notes          a file (unlink)
 *   rm old            an EMPTY directory (rmdir)
 *   rm -r project     a directory and everything in it, deepest first
 *
 * -r walks the tree with one shared path buffer, so each level costs the
 * kernel stack only a few bytes (depth_limit keeps trees 32 deep). What
 * cannot be deleted (open, someone's current directory, not yours) is
 * left, and the directories above it stay too. A symbolic link is
 * removed itself - never followed, not even with -r. Refusals are in `audit`.
 *
 * Uses: sys_unlink / sys_rmdir (syscalls/), vfs_lookup / vfs_readdir /
 *       vfs_lookup_entry (core/vfs), arg_next, strcmp / strlen / strcpy, ui.
 */
#include "modules/filesystem_management/commands/rm/cmd_rm.h"          // this file
#include "modules/filesystem_management/syscalls/unlink/sys_unlink.h"  // sys_unlink
#include "modules/filesystem_management/syscalls/rmdir/sys_rmdir.h"    // sys_rmdir
#include "modules/filesystem_management/core/vfs/vfs.h"                // vfs_lookup, vfs_readdir
#include "lib/args/args.h"                                             // arg_next
#include "lib/string/string.h"                                         // strcmp, strlen, strcpy
#include "modules/ui/core/style/ui.h"                                  // ui_usage, ui_error

static char path[FS_PATH_MAX];      // the one buffer every level adds its name to
static int failed;

// Delete what 'path' names, everything below it first. 'node' is it.
static void remove_tree(vfs_node_t *node) {
    if (node->flags & FS_DIRECTORY) {
        uint32_t len = strlen(path);
        for (uint32_t i = 0; ; ) {
            vfs_node_t *child = vfs_readdir(node, i);
            if (!child)
                break;
            if (len + 1 + strlen(child->name) >= FS_PATH_MAX) {
                i++;                                // cannot even name it: leave it
                failed++;
                continue;
            }
            path[len] = '/';
            strcpy(path + len + 1, child->name);
            int before = node->num_children;
            remove_tree(child);
            path[len] = '\0';
            if (node->num_children == before)
                i++;                                // still there: skip it next time
        }
        if (sys_rmdir(path) != 0)
            failed++;
    } else if (sys_unlink(path) != 0) {
        failed++;
    }
}

void cmd_rm(const char *args) {
    char word[FS_PATH_MAX];
    int recursive = 0;
    const char *rest = arg_next(args, word, sizeof(word));
    if (strcmp(word, "-r") == 0 || strcmp(word, "-rf") == 0) {
        recursive = 1;
        rest = arg_next(rest, word, sizeof(word));
    }
    if (!word[0]) {
        ui_usage("rm [-r] <path>...");
        return;
    }
    for (; word[0]; rest = arg_next(rest, word, sizeof(word))) {
        vfs_node_t *node = vfs_lookup_entry(vfs_cwd(), word);    // a link itself, not where it points
        if (!node) {
            ui_printf("rm: %s: no such file or directory\n", word);
            continue;
        }
        failed = 0;
        strcpy(path, word);
        if (recursive)
            remove_tree(node);
        else if (((node->flags & FS_DIRECTORY) ? sys_rmdir(word) : sys_unlink(word)) != 0)
            failed = 1;
        if (failed)
            ui_printf("rm: %s: %d thing(s) not deleted (in use, not empty, or not yours: see `audit`)\n",
                      word, failed);
    }
}

/*
 * cmd_dirname.c - `dirname <path>`: a path without its last name.
 *
 *   dirname /home/guest/notes.txt   ->  /home/guest
 *   dirname notes.txt               ->  .
 *   dirname /home                   ->  /
 *
 * Pure text: the path need not exist.
 *
 * Uses: strlen / strncpy, ui.
 */
#include "modules/filesystem_management/commands/dirname/cmd_dirname.h"     // this file
#include "modules/filesystem_management/core/vfs/vfs.h"                    // FS_PATH_MAX
#include "lib/string/string.h"                                             // strlen, strncpy
#include "modules/ui/core/style/ui.h"                                      // ui_printf, ui_usage

void cmd_dirname(const char *args) {
    int end = strlen(args);
    if (!end) {
        ui_usage("dirname <path>");
        return;
    }
    while (end > 1 && args[end - 1] == '/')
        end--;                                      // trailing slashes
    while (end > 0 && args[end - 1] != '/')
        end--;                                      // the last name
    while (end > 1 && args[end - 1] == '/')
        end--;                                      // the slashes before it
    if (end == 0) {
        ui_printf(".\n");
        return;
    }
    char out[FS_PATH_MAX];
    if (end >= FS_PATH_MAX)
        end = FS_PATH_MAX - 1;
    strncpy(out, args, end);
    out[end] = '\0';
    ui_printf("%s\n", out);
}

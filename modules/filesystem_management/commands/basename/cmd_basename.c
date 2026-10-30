/*
 * cmd_basename.c - `basename <path>`: the last name of a path.
 *
 *   basename /home/guest/notes.txt   ->  notes.txt
 *   basename /home/guest/            ->  guest
 *   basename /                       ->  /
 *
 * Pure text: the path need not exist.
 *
 * Uses: strlen / strncpy, ui.
 */
#include "modules/filesystem_management/commands/basename/cmd_basename.h"   // this file
#include "modules/filesystem_management/core/vfs/vfs.h"                    // FS_NAME_MAX
#include "lib/string/string.h"                                             // strlen, strncpy
#include "modules/ui/core/style/ui.h"                                      // ui_printf, ui_usage

void cmd_basename(const char *args) {
    int end = strlen(args);
    if (!end) {
        ui_usage("basename <path>");
        return;
    }
    while (end > 1 && args[end - 1] == '/')
        end--;                                      // "a/b/" -> "a/b"
    int start = end;
    while (start > 0 && args[start - 1] != '/')
        start--;
    if (start == end) {                             // only slashes
        ui_printf("/\n");
        return;
    }
    char out[FS_NAME_MAX + 1];
    int n = end - start < FS_NAME_MAX ? end - start : FS_NAME_MAX;
    strncpy(out, args + start, n);                  // "guest/" -> "guest"
    out[n] = '\0';
    ui_printf("%s\n", out);
}

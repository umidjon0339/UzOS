/*
 * cmd_grep.c - `grep <word> <path>`: print the lines that contain a word.
 *
 * The file is read in chunks; characters are collected into 'line'
 * until a newline, then the line is checked. Lines longer than the
 * buffer are cut short for matching.
 *
 * Uses: sys_open / sys_read / sys_close (io_management), arg_next (lib/args),
 *       ui_* (ui module).
 */

#include "modules/filesystem_management/commands/grep/cmd_grep.h"  // this file
#include "modules/io_management/syscalls/open/sys_open.h"          // sys_open, O_READ
#include "modules/io_management/syscalls/read/sys_read.h"          // sys_read
#include "modules/io_management/syscalls/close/sys_close.h"        // sys_close
#include "lib/args/args.h"                                         // arg_next
#include "modules/filesystem_management/core/vfs/vfs.h"            // FS_PATH_MAX
#include "modules/ui/core/style/ui.h"                              // ui_color, UI_GOOD, UI_DIM

#define CHUNK    256
#define LINE_MAX 256

static int contains(const char *text, const char *word) {
    for (; *text; text++) {
        int i = 0;
        while (word[i] && text[i] == word[i])
            i++;
        if (!word[i])
            return 1;
    }
    return 0;
}

static void check_line(char *line, int len, int number, const char *word, int *found) {
    line[len] = '\0';
    if (!contains(line, word))
        return;
    ui_color(UI_GOOD);
    vga_print_int(number);
    ui_color(UI_DIM);
    vga_print_str(": ");
    ui_reset();
    vga_print_str(line);
    vga_print_str("\n");
    (*found)++;
}

void cmd_grep(const char *args) {
    char word[64], path[FS_PATH_MAX];
    arg_next(arg_next(args, word, sizeof(word)), path, sizeof(path));

    int fd = (word[0] && path[0]) ? sys_open(path, O_READ) : -1;
    if (fd < 0) {
        ui_usage("grep <word> <path>   (file must exist)");
        return;
    }

    char buf[CHUNK], line[LINE_MAX + 1];
    int n, len = 0, number = 1, found = 0;

    while ((n = sys_read(fd, buf, CHUNK)) > 0) {
        for (int i = 0; i < n; i++) {
            if (buf[i] == '\n') {
                check_line(line, len, number++, word, &found);
                len = 0;
            } else if (len < LINE_MAX) {
                line[len++] = buf[i];
            }
        }
    }
    if (len)
        check_line(line, len, number, word, &found);   // last line, no '\n'
    sys_close(fd);

    if (!found)
        vga_print_str("(no matches)\n");
}

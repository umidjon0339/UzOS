/*
 * cmd_wc.c - `wc <path>`: count lines, words and bytes.
 *
 * A word is a run of characters between spaces, tabs or newlines.
 *
 * Uses: sys_open / sys_read / sys_close (io_management), ui_* (ui module).
 */

#include "modules/filesystem_management/commands/wc/cmd_wc.h"  // this file
#include "modules/io_management/syscalls/open/sys_open.h"      // sys_open, O_READ
#include "modules/io_management/syscalls/read/sys_read.h"      // sys_read
#include "modules/io_management/syscalls/close/sys_close.h"    // sys_close
#include "modules/ui/core/style/ui.h"                          // ui_usage

#define CHUNK 512

static int is_space(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

void cmd_wc(const char *args) {
    int fd = args[0] ? sys_open(args, O_READ) : -1;
    if (fd < 0) {
        ui_usage("wc <path>   (file must exist)");
        return;
    }

    char buf[CHUNK];
    int n, in_word = 0;
    uint32_t lines = 0, words = 0, bytes = 0;

    while ((n = sys_read(fd, buf, CHUNK)) > 0) {
        bytes += n;
        for (int i = 0; i < n; i++) {
            if (buf[i] == '\n')
                lines++;
            if (is_space(buf[i]))
                in_word = 0;
            else if (!in_word) {
                in_word = 1;        // the first letter of a new word
                words++;
            }
        }
    }
    sys_close(fd);

    vga_print_str("lines ");
    vga_print_int(lines);
    vga_print_str("  words ");
    vga_print_int(words);
    vga_print_str("  bytes ");
    vga_print_int(bytes);
    vga_print_str("\n");
}

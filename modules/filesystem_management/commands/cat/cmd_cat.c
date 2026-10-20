/*
 * cmd_cat.c - `cat <path>`: print a whole file.
 *
 * The classic Unix loop: read a chunk from the file, write it to standard
 * output (fd 1), until read() returns 0 (end of file).
 *
 * Uses: sys_open / sys_read / sys_write / sys_close and STDOUT (io_management),
 *       ui_* (ui module).
 */

#include "fs_commands.h"
#include "sys_open.h"
#include "sys_read.h"
#include "sys_write.h"
#include "sys_close.h"
#include "fd.h"
#include "ui.h"

#define CHUNK 256

void cmd_cat(const char *args) {
    if (!args[0]) {
        ui_usage("cat <path>");
        return;
    }
    int fd = sys_open(args, O_READ);
    if (fd < 0) {
        ui_error("cat", "cannot open (no such file?)");
        return;
    }

    char buf[CHUNK];
    int n, last = '\n';
    while ((n = sys_read(fd, buf, CHUNK)) > 0) {
        sys_write(STDOUT, buf, n);
        last = buf[n - 1];
    }
    sys_close(fd);

    if (last != '\n')
        vga_print_str("\n");        // keep the prompt on its own line
}

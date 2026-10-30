/*
 * cmd_cat.c - `cat <path>`: print a whole file.
 *
 * The classic Unix loop: read a chunk from the file, write it to standard
 * output (fd 1), until read() returns 0 (end of file).
 *
 * Uses: sys_open / sys_read / sys_write / sys_close and STDOUT (io_management),
 *       ui_* (ui module).
 */

#include "modules/filesystem_management/commands/cat/cmd_cat.h"  // this file
#include "modules/io_management/syscalls/open/sys_open.h"        // sys_open, O_READ
#include "modules/io_management/syscalls/read/sys_read.h"        // sys_read
#include "modules/io_management/syscalls/write/sys_write.h"      // sys_write
#include "modules/io_management/syscalls/close/sys_close.h"      // sys_close
#include "modules/io_management/core/fd/fd.h"                    // STDOUT
#include "modules/ui/core/style/ui.h"                            // ui_usage, ui_error

#define CHUNK 256

void cmd_cat(const char *args) {
    if (!args[0]) {
        ui_usage("cat <path>");
        return;
    }
    int fd = sys_open(args, O_READ);
    if (fd < 0) {
        ui_error("cat", "cannot open (no such file, or permission denied)");
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

/*
 * cmd_ioinfo.c - `ioinfo [pid]`: a process's file descriptors.
 *
 * Without a PID it shows the shell itself. For each open file: its
 * number, how it was opened (r = read, w = write, a = append), where the
 * next read/write happens, and the file's path.
 *
 * Uses: the fd table (process_t), process_get_by_pid / the current process
 *       (process_management), vfs_path_of (filesystem_management), atoi,
 *       box_* / ui_cell (ui module).
 */

#include "io_commands.h"
#include "fd.h"
#include "sys_open.h"
#include "scheduler.h"
#include "stdlib.h"
#include "format.h"
#include "ui.h"
#include "box.h"

#define WIDTH 60

static void row(int fd, const char *mode, const char *offset, const char *file) {
    box_row();
    ui_printf("%3d  %-4s %8s  ", fd, mode, offset);
    ui_cell(file, 36);
    box_row_end();
}

static void print_file(const process_t *p, int fd) {
    char path[FS_PATH_MAX], offset[12];
    uint32_t flags = p->fd_flags[fd];
    char mode[4] = {
        flags & O_READ   ? 'r' : '-',
        flags & O_WRITE  ? 'w' : '-',
        flags & O_APPEND ? 'a' : '-',
        '\0'
    };
    format(offset, sizeof(offset), "%u", p->fd_offset[fd]);
    row(fd, mode, offset, vfs_path_of(p->fd_table[fd], path, sizeof(path)) == 0 ? path : "?");
}

void cmd_ioinfo(const char *args) {
    process_t *p = args[0] ? process_get_by_pid(atoi(args)) : scheduler_get_current_process();
    if (!p) {
        ui_error("ioinfo", "no such process");
        return;
    }

    char title[48];
    format(title, sizeof(title), "Descriptors of PID %u", p->pid);
    box_begin(title, WIDTH);
    box_row();
    ui_color(UI_TITLE);
    ui_printf(" fd  mode   offset  file");
    box_row_end();

    row(0, "r--", "-", "stdin  (keyboard)");
    row(1, "-w-", "-", "stdout (screen)");
    row(2, "-w-", "-", "stderr (screen)");
    for (int fd = FIRST_FILE_FD; fd < MAX_FDS; fd++)
        if (p->fd_table[fd])
            print_file(p, fd);

    box_row();
    ui_color(UI_DIM);
    ui_printf("open files: %d of %d", fd_count(p), MAX_FDS - FIRST_FILE_FD);
    box_row_end();
    box_end();
}

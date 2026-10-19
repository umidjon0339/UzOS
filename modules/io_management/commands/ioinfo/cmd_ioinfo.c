#include "../io_commands_internal.h"
#include "io_security.h"
#include "scheduler.h"
#include "vga.h"
#include "vfs.h"

static void print_descriptor(int fd, const char *kind, const char *name) {
    vga_print_str("  fd ");
    vga_print_int(fd);
    vga_print_str(": ");
    vga_print_str(kind);
    if (name) {
        vga_print_str(" (");
        vga_print_str(name);
        vga_print_str(")");
    }
    vga_print_str("\n");
}

void cmd_ioinfo(const char *args) {
    (void)args;
    process_t *current = scheduler_get_current_process();
    if (!current) {
        vga_print_str("I/O status unavailable: no current process.\n");
        return;
    }

    vga_print_str("\nI/O descriptors for PID ");
    vga_print_int((int)current->pid);
    vga_print_str("\n");
    print_descriptor(IO_STDIN, "standard input", "keyboard");
    print_descriptor(IO_STDOUT, "standard output", "terminal");
    print_descriptor(IO_STDERR, "standard error", "terminal");

    int open_files = 0;
    for (int fd = IO_FIRST_FILE_FD; fd < MAX_FDS; fd++) {
        vfs_node_t *node = current->fd_table[fd];
        if (!node) continue;
        print_descriptor(fd, "file", node->name);
        open_files++;
    }

    vga_print_str("  open files: ");
    vga_print_int(open_files);
    vga_print_str("\n");
}

#include "../fs_commands_internal.h"
#include "fs_syscalls.h"
#include "vga.h"

void cmd_cat(const char *args) {
    if (args[0] == '\0') {
        vga_print_str("Usage: cat <file>\n");
        return;
    }
    
    int fd = sys_open(args);
    if (fd == -1) {
        vga_print_str("Failed to open file.\n");
        return;
    }
    
    char buffer[256];
    int read = sys_read(fd, buffer, 255);
    sys_close(fd);
    
    if (read >= 0) {
        buffer[read] = '\0';
        vga_print_str(buffer);
        vga_print_str("\n");
    } else {
        vga_print_str("Error reading file.\n");
    }
}

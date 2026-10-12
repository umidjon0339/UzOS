#include "../fs_commands_internal.h"
#include "fs_syscalls.h"
#include "vga.h"

void cmd_rm(const char *args) {
    if (args[0] == '\0') {
        vga_print_str("Usage: rm <file/dir>\n");
        return;
    }
    
    if (sys_unlink(args) == 0) {
        vga_print_str("Deleted successfully.\n");
    } else {
        vga_print_str("Failed to delete (not found).\n");
    }
}

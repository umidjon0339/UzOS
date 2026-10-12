#include "../fs_commands_internal.h"
#include "fs_syscalls.h"
#include "vga.h"

void cmd_mkdir(const char *args) {
    if (args[0] == '\0') {
        vga_print_str("Usage: mkdir <name>\n");
        return;
    }
    
    if (sys_mkdir(args) == 0) {
        vga_print_str("Directory created.\n");
    } else {
        vga_print_str("Failed to create directory.\n");
    }
}

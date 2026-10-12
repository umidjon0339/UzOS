#include "../fs_commands_internal.h"
#include "fs_syscalls.h"
#include "vga.h"

void cmd_pwd(const char *args) {
    (void)args;
    char path[256];
    if (sys_getcwd_string(path, 256) == 0) {
        vga_print_str(path);
        vga_print_str("\n");
    } else {
        vga_print_str("Error resolving path\n");
    }
}

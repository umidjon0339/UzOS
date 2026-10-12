#include "../fs_commands_internal.h"
#include "fs_syscalls.h"
#include "vga.h"

void cmd_cd(const char *args) {
    if (sys_chdir(args) == 0) {
        // Success
    } else {
        vga_print_str("cd: directory not found.\n");
    }
}

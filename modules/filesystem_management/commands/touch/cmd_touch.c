#include "../fs_commands_internal.h"
#include "fs_syscalls.h"
#include "ramdisk.h"
#include "vga.h"

void cmd_touch(const char *args) {
    if (args[0] == '\0') {
        vga_print_str("Usage: touch <name>\n");
        return;
    }
    
    vfs_node_t *dir = sys_getcwd_node();
    if (ramdisk_create_file(dir, args) != 0) {
        vga_print_str("File created.\n");
    } else {
        vga_print_str("Failed to create file.\n");
    }
}

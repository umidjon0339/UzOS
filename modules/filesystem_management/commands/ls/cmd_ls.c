#include "../fs_commands_internal.h"
#include "fs_syscalls.h"
#include "vfs.h"
#include "vga.h"

void cmd_ls(const char *args) {
    vfs_node_t *dir = sys_getcwd_node();
    
    // If user provided an argument like "ls my_folder", try to find it
    if (args[0] != '\0') {
        vfs_node_t *target = vfs_finddir(dir, args);
        if (target && target->flags == FS_DIRECTORY) {
            dir = target;
        } else {
            vga_print_str("ls: cannot access '");
            vga_print_str(args);
            vga_print_str("': No such file or directory\n");
            return;
        }
    }
    
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print_str("Type\tSize\tName\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    int i = 0;
    struct vfs_node *child = 0;
    while ((child = vfs_readdir(dir, i)) != 0) {
        if (child->flags == FS_DIRECTORY) {
            vga_print_str("DIR\t");
            vga_print_str("-\t");
        } else {
            vga_print_str("FILE\t");
            vga_print_int(child->size);
            vga_print_str("\t");
        }
        vga_print_str(child->name);
        vga_print_str("\n");
        i++;
    }
    
    if (i == 0) {
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        vga_print_str("(empty)\n");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    }
}

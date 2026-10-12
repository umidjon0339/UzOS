#include "../fs_commands_internal.h"
#include "fs_syscalls.h"
#include "vfs.h"
#include "ramdisk.h"
#include "string.h"
#include "vga.h"
#include "kheap.h"

// cp <source> <dest> — Copy a file in the current directory
void cmd_cp(const char *args) {
    if (args[0] == '\0') {
        vga_print_str("Usage: cp <source> <dest>\n");
        return;
    }

    // Parse source and dest names
    char src_name[128];
    char dst_name[128];
    int i = 0;

    // Parse source
    while (args[i] && args[i] != ' ' && i < 127) {
        src_name[i] = args[i];
        i++;
    }
    src_name[i] = '\0';

    if (args[i] != ' ') {
        vga_print_str("Usage: cp <source> <dest>\n");
        return;
    }
    i++; // skip space

    // Parse dest
    int j = 0;
    while (args[i] && args[i] != ' ' && j < 127) {
        dst_name[j++] = args[i++];
    }
    dst_name[j] = '\0';

    if (dst_name[0] == '\0') {
        vga_print_str("Usage: cp <source> <dest>\n");
        return;
    }

    // Open source file and read its contents
    int fd = sys_open(src_name);
    if (fd == -1) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_print_str("Error: Cannot open source file '");
        vga_print_str(src_name);
        vga_print_str("'\n");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return;
    }

    // Read source file into a temporary buffer (up to 16KB — ramdisk max)
    char *buffer = kmalloc(16384);
    if (!buffer) {
        sys_close(fd);
        vga_print_str("Error: Out of memory\n");
        return;
    }

    int bytes_read = sys_read(fd, buffer, 16384);
    sys_close(fd);

    if (bytes_read < 0) {
        kfree(buffer);
        vga_print_str("Error: Failed to read source file\n");
        return;
    }

    // Create destination file in current directory
    vfs_node_t *cwd = sys_getcwd_node();
    if (!cwd) {
        kfree(buffer);
        vga_print_str("Error: Cannot get current directory\n");
        return;
    }

    vfs_node_t *dst_file = ramdisk_create_file(cwd, dst_name);
    if (!dst_file) {
        kfree(buffer);
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_print_str("Error: Cannot create '");
        vga_print_str(dst_name);
        vga_print_str("' (already exists?)\n");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return;
    }

    // Write to destination
    if (bytes_read > 0) {
        vfs_write(dst_file, 0, bytes_read, (uint8_t *)buffer);
    }

    kfree(buffer);

    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print_str("Copied '");
    vga_print_str(src_name);
    vga_print_str("' -> '");
    vga_print_str(dst_name);
    vga_print_str("'\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}

// mv <source> <dest> — Move/rename a file in the current directory
void cmd_mv(const char *args) {
    if (args[0] == '\0') {
        vga_print_str("Usage: mv <source> <dest>\n");
        return;
    }

    // Parse source and dest names
    char src_name[128];
    char dst_name[128];
    int i = 0;

    // Parse source
    while (args[i] && args[i] != ' ' && i < 127) {
        src_name[i] = args[i];
        i++;
    }
    src_name[i] = '\0';

    if (args[i] != ' ') {
        vga_print_str("Usage: mv <source> <dest>\n");
        return;
    }
    i++; // skip space

    // Parse dest
    int j = 0;
    while (args[i] && args[i] != ' ' && j < 127) {
        dst_name[j++] = args[i++];
    }
    dst_name[j] = '\0';

    if (dst_name[0] == '\0') {
        vga_print_str("Usage: mv <source> <dest>\n");
        return;
    }

    // Find source file in current directory
    vfs_node_t *cwd = sys_getcwd_node();
    if (!cwd) {
        vga_print_str("Error: Cannot get current directory\n");
        return;
    }

    vfs_node_t *src_node = vfs_finddir(cwd, src_name);
    if (!src_node) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_print_str("Error: '");
        vga_print_str(src_name);
        vga_print_str("' not found\n");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return;
    }

    // Check destination doesn't already exist
    if (vfs_finddir(cwd, dst_name) != 0) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_print_str("Error: '");
        vga_print_str(dst_name);
        vga_print_str("' already exists\n");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return;
    }

    // Simply rename the node (mv is just a rename in the same directory)
    strncpy(src_node->name, dst_name, 127);
    src_node->name[127] = '\0';

    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print_str("Moved '");
    vga_print_str(src_name);
    vga_print_str("' -> '");
    vga_print_str(dst_name);
    vga_print_str("'\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}

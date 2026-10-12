#include "../fs_commands_internal.h"
#include "fs_syscalls.h"
#include "string.h"
#include "vga.h"

void cmd_write(const char *args) {
    // Usage: write file "text"
    char filename[64];
    int i = 0;
    while (args[i] && args[i] != ' ' && i < 63) {
        filename[i] = args[i];
        i++;
    }
    filename[i] = '\0';
    
    if (args[i] != ' ') {
        vga_print_str("Usage: write <file> <text>\n");
        return;
    }
    
    const char *text = &args[i + 1];
    
    int fd = sys_open(filename);
    if (fd == -1) {
        vga_print_str("Failed to open file (make sure it exists with 'touch').\n");
        return;
    }
    
    int written = sys_write(fd, text, strlen(text));
    sys_close(fd);
    
    if (written >= 0) {
        vga_print_str("Wrote to file.\n");
    } else {
        vga_print_str("Error writing to file.\n");
    }
}

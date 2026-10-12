#include "../proc_commands_internal.h"
#include "process.h"
#include "vga.h"

void cmd_kill(const char *args) {
    if (args[0] == '\0') {
        vga_print_str("Usage: kill <pid>\n");
        return;
    }
    
    int pid = simple_atoi(args);
    if (pid <= 0) {
        vga_print_str("Invalid PID.\n");
        return;
    }
    
    if (pid == 1) { // Assuming PID 1 is the shell/idle process
        vga_print_str("Cannot kill the idle/shell process.\n");
        return;
    }
    
    process_kill(pid);
    vga_print_str("Process killed.\n");
}

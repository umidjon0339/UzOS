#include "../proc_commands_internal.h"
#include "vga.h"
#include "elf.h"

void cmd_exec(const char *args) {
    if (args[0] == '\0') {
        vga_print_str("Usage: exec <filename>\n");
        return;
    }
    
    int pid = elf_load_and_run(args);
    if (pid >= 0) {
        vga_print_str("Started process PID ");
        vga_print_int(pid);
        vga_print_str("\n");
    }
}

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

    /* The real protection (reserved PIDs, ownership) lives in the
     * kernel's kill guard, not here. A check written in the shell would
     * be bypassed by any other caller. */
    if (process_kill(pid)) {
        vga_print_str("Process killed.\n");
    } else {
        vga_print_str("Kill refused by the security guard.\n");
    }
}

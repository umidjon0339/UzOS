#include "../proc_commands_internal.h"
#include "vga.h"
#include "proc_syscalls.h"

void cmd_sleep(const char *args) {
    if (args[0] == '\0') {
        vga_print_str("Usage: sleep <seconds>\n");
        return;
    }
    int seconds = simple_atoi(args);
    if (seconds <= 0) return;
    sys_sleep(seconds * 1000);
}

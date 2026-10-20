/*
 * cmd_sleep.c - `sleep <seconds>`: pause the shell.
 *
 * Uses: sys_sleep (syscalls/sleep), atoi (lib/stdlib), ui_* (ui module).
 */

#include "proc_commands.h"
#include "sys_sleep.h"
#include "stdlib.h"
#include "ui.h"

void cmd_sleep(const char *args) {
    int seconds = atoi(args);

    if (seconds <= 0) {
        ui_usage("sleep <seconds>");
        return;
    }
    sys_sleep(seconds * 1000);
}

/*
 * cmd_sleep.c - `sleep <seconds>`: pause the shell. Ctrl+C ends it early.
 *
 * Uses: sys_sleep_interruptible (syscalls/sleep), atoi (lib/stdlib),
 *       ui_* (ui module).
 */

#include "modules/process_management/commands/sleep/cmd_sleep.h"  // this file
#include "modules/process_management/syscalls/sleep/sys_sleep.h"  // sys_sleep_interruptible
#include "lib/stdlib/stdlib.h"                                    // atoi
#include "modules/ui/core/style/ui.h"                             // ui_usage

void cmd_sleep(const char *args) {
    int seconds = atoi(args);

    if (seconds <= 0) {
        ui_usage("sleep <seconds>");
        return;
    }
    sys_sleep_interruptible(seconds * 1000);   // a signal (Ctrl+C) wakes it
}

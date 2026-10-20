/*
 * cmd_kill.c - `kill <pid>`: stop a process.
 *
 * Uses: process_kill (core/process), atoi (lib/stdlib), ui_* (ui module).
 * Why the command checks almost nothing: the real rules (no system
 * processes, only your own) live in kill_guard INSIDE process_kill(),
 * so a syscall or another module cannot skip them.
 */

#include "proc_commands.h"
#include "process.h"
#include "stdlib.h"
#include "ui.h"

void cmd_kill(const char *args) {
    if (args[0] == '\0') {
        ui_usage("kill <pid>");
        return;
    }

    int pid = atoi(args);
    if (pid <= 0) {
        ui_error("kill", "not a valid PID");
        return;
    }

    if (process_kill(pid))
        ui_ok("kill", "process stopped");
    else
        ui_error("kill", "refused: no such process, or a system one (see `seclab log`)");
}

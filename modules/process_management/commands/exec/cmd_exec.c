/*
 * cmd_exec.c - `exec <file>`: load an ELF program and run it.
 *
 * Uses: elf_load_and_run (core/elf), ui_* (ui module). Try `exec hello`.
 */

#include "proc_commands.h"
#include "elf.h"
#include "ui.h"

void cmd_exec(const char *args) {
    if (args[0] == '\0') {
        ui_usage("exec <filename>");
        return;
    }

    int pid = elf_load_and_run(args);
    if (pid < 0)
        return;     // the loader already printed why

    ui_start(UI_OK, "exec");
    ui_printf("started the program, PID %d", pid);
    ui_end();
}

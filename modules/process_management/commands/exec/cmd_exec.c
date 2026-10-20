/*
 * cmd_exec.c - `exec <file> [&]`: load an ELF program and run it.
 *
 *   exec hello       run it in the FOREGROUND: the shell waits until the
 *                    program ends; Ctrl+C stops it
 *   exec hello &     run it in the BACKGROUND: the prompt comes back at
 *                    once (stop it later with `kill <pid>`)
 *
 * Ctrl+C sends SIGINT to the shell, which is running this command. We
 * pass it on the way a terminal does: the foreground program is ended.
 * The kill goes through process_kill(), so kill_guard still decides.
 *
 * Uses: elf_load_and_run (core/elf), process_get_by_pid / process_kill
 *       (core/process), sys_sleep_interruptible (syscalls/sleep),
 *       arg_next (lib/args), ui_* (ui module). Try `exec hello`.
 */

#include "proc_commands.h"
#include "elf.h"
#include "process.h"
#include "sys_sleep.h"
#include "args.h"
#include "string.h"
#include "ui.h"

#define WAIT_STEP_MS 50     // how often the shell looks whether it ended

static int still_running(int pid) {
    process_t *p = process_get_by_pid(pid);
    return p && p->state != PROCESS_STATE_ZOMBIE;
}

// Wait for the foreground program. Ctrl+C ends it.
static void wait_foreground(int pid) {
    while (still_running(pid)) {
        if (sys_sleep_interruptible(WAIT_STEP_MS))
            continue;                       // slept normally: look again

        // Woken by a signal: Ctrl+C. Stop the program.
        if (process_kill(pid)) {
            ui_start(UI_INFO, "exec");
            ui_printf("stopped PID %d", pid);
            ui_end();
        } else {
            ui_error("exec", "could not stop it (see `seclab log`)");
        }
        return;
    }
    ui_start(UI_OK, "exec");
    ui_printf("PID %d finished", pid);
    ui_end();
}

void cmd_exec(const char *args) {
    char file[64], amp[4];
    const char *rest = arg_next(args, file, sizeof(file));
    arg_next(rest, amp, sizeof(amp));

    int background = strcmp(amp, "&") == 0;
    int len = strlen(file);
    if (len > 1 && file[len - 1] == '&') {  // "exec hello&" works too
        file[len - 1] = '\0';
        background = 1;
    }

    if (file[0] == '\0' || (amp[0] && !background)) {
        ui_usage("exec <file> [&]");
        return;
    }

    int pid = elf_load_and_run(file);
    if (pid < 0)
        return;     // the loader already printed why

    ui_start(UI_OK, "exec");
    if (background)
        ui_printf("started PID %d in the background (kill %d stops it)", pid, pid);
    else
        ui_printf("running PID %d (Ctrl+C stops it)", pid);
    ui_end();

    if (!background)
        wait_foreground(pid);
}

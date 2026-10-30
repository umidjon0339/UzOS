/*
 * cmd_exec.c - `exec <file> [&]`: load an ELF program and run it.
 *
 *   exec hello       run it in the FOREGROUND: the shell waits until the
 *                    program ends; Ctrl+C ends it, Ctrl+Z stops it (a
 *                    stopped job: `jobs`, `fg`, `bg`)
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

#include "modules/process_management/commands/exec/cmd_exec.h"    // this file
#include "modules/process_management/core/elf/elf.h"              // elf_load_and_run
#include "modules/process_management/core/scheduler/scheduler.h"  // scheduler_get_current_process
#include "modules/process_management/core/signal/signal.h"        // signal_pending, SIGTSTP, SIGSTOP
#include "modules/process_management/core/process/process.h"      // process_t, process_get_by_pid
#include "modules/process_management/syscalls/sleep/sys_sleep.h"  // sys_sleep_interruptible
#include "lib/args/args.h"                                        // arg_next
#include "lib/string/string.h"                                    // strcmp, strlen
#include "modules/ui/core/style/ui.h"                             // ui_start, ui_printf, ui_end

#define WAIT_STEP_MS 50     // how often the shell looks whether it ended

static int still_running(int pid) {
    process_t *p = process_get_by_pid(pid);
    return p && p->state != PROCESS_STATE_ZOMBIE;
}

// Wait for the foreground program. Ctrl+C ends it, Ctrl+Z stops it.
static void wait_foreground(int pid) {
    process_t *self = scheduler_get_current_process();
    while (still_running(pid)) {
        if (sys_sleep_interruptible(WAIT_STEP_MS))
            continue;                       // slept normally: look again

        if (signal_pending(self, SIGTSTP)) {    // Ctrl+Z: freeze it, prompt back
            signal_clear(self, SIGTSTP);
            signal_send(process_get_by_pid(pid), SIGSTOP);
            ui_start(UI_INFO, "exec");
            ui_printf("stopped PID %d (`fg` continues it, `bg` runs it in the background)", pid);
            ui_end();
            return;
        }
        if (!signal_pending(self, SIGINT))
            continue;                       // some other signal: keep waiting

        // Ctrl+C: end the program.
        if (process_kill(pid)) {
            ui_start(UI_INFO, "exec");
            ui_printf("stopped PID %d", pid);
            ui_end();
        } else {
            ui_error("exec", "could not stop it (see `audit`)");
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
